#include "base/std.h"
#include "compile_service.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#include <sddl.h>
#endif
#include <nlohmann/json.hpp>

#include "backend.h"
#include "compile_service_protocol.h"
#include "runtime_compile_request.h"

namespace {

std::atomic<bool> g_running{false};
std::atomic<bool> g_stopping{false};
std::thread g_server_thread;
std::mutex g_client_threads_mutex;
std::vector<std::thread> g_client_threads;
std::string g_service_id;
std::string g_pipe_name;
std::atomic<unsigned long> g_last_pipe_error{0};
std::mutex g_queue_mutex;

struct PendingRequest {
  compile_service::CompileServiceRequest request;
  compile_service::CompileServiceResponse response;
  bool completed = false;
  std::chrono::steady_clock::time_point enqueued_at = std::chrono::steady_clock::now();
  std::condition_variable done_cv;
};

std::deque<std::shared_ptr<PendingRequest>> g_request_queue;
compile_service::RequestExecutor g_request_executor = compile_service::execute_compile_service_request;

constexpr auto kQueueTimeout = std::chrono::seconds(5);
constexpr int kQueueTimeoutMs = 5000;
#ifdef _WIN32
constexpr const char *kPipeSecurityDescriptor =
    "D:(A;;GA;;;WD)(A;;GA;;;AU)(A;;GA;;;SY)(A;;GA;;;BA)";

class PipeSecurityAttributes {
 public:
  PipeSecurityAttributes() {
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorA(
            kPipeSecurityDescriptor, SDDL_REVISION_1, &descriptor_, nullptr)) {
      g_last_pipe_error = GetLastError();
      debug_message("compile_service: pipe security descriptor setup failed (win32=%lu)\n",
                    g_last_pipe_error.load());
      return;
    }

    attributes_.nLength = sizeof(attributes_);
    attributes_.lpSecurityDescriptor = descriptor_;
    attributes_.bInheritHandle = FALSE;
  }

  ~PipeSecurityAttributes() {
    if (descriptor_ != nullptr) {
      LocalFree(descriptor_);
    }
  }

  SECURITY_ATTRIBUTES *get() { return descriptor_ != nullptr ? &attributes_ : nullptr; }

 private:
  SECURITY_ATTRIBUTES attributes_{};
  PSECURITY_DESCRIPTOR descriptor_ = nullptr;
};
#endif  // _WIN32

std::string compile_request_kind_from_target(std::string_view target) {
  if (!target.empty() && (target.back() == '/' || target.back() == '\\')) {
    return "directory";
  }
  return "file";
}

compile_service::CompileServiceResponse dispatch_compile_service_request(
    const compile_service::CompileServiceRequest &request) {
  auto pending = std::make_shared<PendingRequest>();
  pending->request = request;

  {
    std::lock_guard<std::mutex> lock(g_queue_mutex);
    g_request_queue.push_back(pending);
  }

  std::unique_lock<std::mutex> lock(g_queue_mutex);
  pending->done_cv.wait(lock, [&] { return pending->completed; });
  return pending->response;
}

void complete_pending_request(std::shared_ptr<PendingRequest> pending,
                              compile_service::CompileServiceResponse response) {
  {
    std::lock_guard<std::mutex> lock(g_queue_mutex);
    pending->response = std::move(response);
    pending->completed = true;
  }
  pending->done_cv.notify_one();
}

compile_service::CompileServiceResponse build_service_stopped_response(
    const compile_service::CompileServiceRequest &request) {
  compile_service::CompileServiceResponse response;
  response.version = 1;
  response.kind = request.op == "dev_test" ? "dev_test" : compile_request_kind_from_target(request.target);
  response.target = request.target;
  compile_service::set_compile_service_failure(
      &response, "connect", "service_error",
      "compile service stopped before this request could be processed");
  if (request.op == "dev_test") {
    response.compile_status = "unknown";
    response.test_status = "not_run";
    response.test_message = response.message;
    response.error = nlohmann::json{{"type", response.reason}, {"message", response.message}};
  } else {
    compile_service::add_compile_service_diagnostic(
        &response, "error", response.kind == "file" ? response.target : std::string{}, 0, 0,
        response.message);
  }
  return response;
}

int process_compile_service_requests(int max_requests) {
  if (!g_running.load() || g_stopping.load()) {
    return 0;
  }

  int processed = 0;
  while (max_requests <= 0 || processed < max_requests) {
    std::shared_ptr<PendingRequest> pending;
    {
      std::lock_guard<std::mutex> lock(g_queue_mutex);
      if (g_request_queue.empty()) {
        break;
      }
      pending = g_request_queue.front();
      g_request_queue.pop_front();
    }

    compile_service::CompileServiceResponse response;
    auto wait_time = std::chrono::steady_clock::now() - pending->enqueued_at;
    if (wait_time > kQueueTimeout) {
      if (pending->request.op == "dev_test") {
        response = compile_service::build_dev_test_queue_timeout_response(pending->request.target,
                                                                          kQueueTimeoutMs);
      } else {
        response = compile_service::build_compile_queue_timeout_response(
            compile_request_kind_from_target(pending->request.target), pending->request.target,
            kQueueTimeoutMs);
      }
    } else {
      compile_service::RequestExecutor executor;
      {
        std::lock_guard<std::mutex> lock(g_queue_mutex);
        executor = g_request_executor;
      }
      response = executor(pending->request);
    }

    complete_pending_request(std::move(pending), std::move(response));
    processed++;
  }
  return processed;
}

void fail_queued_requests() {
  std::deque<std::shared_ptr<PendingRequest>> pending_requests;
  {
    std::lock_guard<std::mutex> lock(g_queue_mutex);
    pending_requests.swap(g_request_queue);
  }

  for (auto &pending : pending_requests) {
    complete_pending_request(pending, build_service_stopped_response(pending->request));
  }
}

#ifdef _WIN32
void handle_client(HANDLE pipe) {
  std::string request_text;
  char buffer[4096];
  DWORD bytes_read = 0;
  while (ReadFile(pipe, buffer, sizeof buffer, &bytes_read, nullptr) && bytes_read > 0) {
    request_text.append(buffer, buffer + bytes_read);
    auto pos = request_text.find('\n');
    if (pos != std::string::npos) {
      request_text.resize(pos);
      break;
    }
  }

  if (!request_text.empty() && !g_stopping.load()) {
    auto request = nlohmann::json::parse(request_text).get<compile_service::CompileServiceRequest>();
    auto response = dispatch_compile_service_request(request);
    auto payload = nlohmann::json(response).dump() + "\n";
    DWORD bytes_written = 0;
    WriteFile(pipe, payload.data(), static_cast<DWORD>(payload.size()), &bytes_written, nullptr);
    FlushFileBuffers(pipe);
  }

  DisconnectNamedPipe(pipe);
  CloseHandle(pipe);
}

void service_loop() {
  debug_message("compile_service: service loop starting for %s\n", g_pipe_name.c_str());
  PipeSecurityAttributes pipe_security;
  while (!g_stopping.load()) {
    HANDLE pipe =
        CreateNamedPipeA(g_pipe_name.c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                         PIPE_UNLIMITED_INSTANCES, 64 * 1024, 64 * 1024, 0, pipe_security.get());
    if (pipe == INVALID_HANDLE_VALUE) {
      g_last_pipe_error = GetLastError();
      debug_message("compile_service: CreateNamedPipeA failed for %s (win32=%lu)\n", g_pipe_name.c_str(),
                    g_last_pipe_error.load());
      return;
    }
    debug_message("compile_service: waiting for client on %s\n", g_pipe_name.c_str());

    BOOL connected = ConnectNamedPipe(pipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
    if (!connected) {
      g_last_pipe_error = GetLastError();
      debug_message("compile_service: ConnectNamedPipe failed for %s (win32=%lu)\n", g_pipe_name.c_str(),
                    g_last_pipe_error.load());
      CloseHandle(pipe);
      continue;
    }
    debug_message("compile_service: client connected on %s\n", g_pipe_name.c_str());
    std::lock_guard<std::mutex> lock(g_client_threads_mutex);
    g_client_threads.emplace_back([pipe] { handle_client(pipe); });
  }
}
#endif  // _WIN32

}  // namespace

bool start_compile_service(std::string_view config_path) {
  if (g_running.load()) {
    return true;
  }

  g_service_id = compile_service::make_compile_service_id(config_path);
  g_pipe_name = compile_service::make_compile_service_pipe_name(g_service_id);
  debug_message("compile_service: starting id=%s pipe=%s\n", g_service_id.c_str(), g_pipe_name.c_str());
  g_stopping = false;
  g_running = true;
#ifdef _WIN32
  g_server_thread = std::thread(service_loop);
#endif
  return true;
}

void stop_compile_service() {
  if (!g_running.load()) {
    return;
  }

  g_stopping = true;
  fail_queued_requests();
#ifdef _WIN32
  HANDLE pipe =
      CreateFileA(g_pipe_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
  if (pipe != INVALID_HANDLE_VALUE) {
    CloseHandle(pipe);
  }
#endif

  if (g_server_thread.joinable()) {
    g_server_thread.join();
  }
  {
    std::lock_guard<std::mutex> lock(g_client_threads_mutex);
    for (auto &thread : g_client_threads) {
      if (thread.joinable()) {
        thread.join();
      }
    }
    g_client_threads.clear();
  }
  g_running = false;
}

bool compile_service_running() { return g_running.load(); }

std::string compile_service_id() { return g_service_id; }

namespace compile_service {

CompileServiceResponse dispatch_compile_service_request_for_testing(const CompileServiceRequest &request) {
  return ::dispatch_compile_service_request(request);
}

int process_compile_service_requests_on_main_thread(int max_requests) {
  return ::process_compile_service_requests(max_requests);
}

int process_compile_service_requests_for_testing(int max_requests) {
  return ::process_compile_service_requests(max_requests);
}

void set_compile_service_request_executor_for_testing(RequestExecutor executor) {
  std::lock_guard<std::mutex> lock(g_queue_mutex);
  g_request_executor = std::move(executor);
}

void clear_compile_service_request_executor_for_testing() {
  std::lock_guard<std::mutex> lock(g_queue_mutex);
  g_request_executor = execute_compile_service_request;
}

}  // namespace compile_service
