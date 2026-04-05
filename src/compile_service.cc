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

#include <windows.h>
#include <nlohmann/json.hpp>

#include "backend.h"
#include "compile_service_protocol.h"
#include "runtime_compile_request.h"

namespace {

std::atomic<bool> g_running{false};
std::atomic<bool> g_stopping{false};
std::thread g_server_thread;
std::thread g_worker_thread;
std::mutex g_client_threads_mutex;
std::vector<std::thread> g_client_threads;
std::string g_service_id;
std::string g_pipe_name;
std::atomic<unsigned long> g_last_pipe_error{0};
std::mutex g_queue_mutex;
std::condition_variable g_queue_cv;

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
  g_queue_cv.notify_one();

  std::unique_lock<std::mutex> lock(g_queue_mutex);
  pending->done_cv.wait(lock, [&] { return pending->completed; });
  return pending->response;
}

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

void request_worker_loop() {
  while (true) {
    std::shared_ptr<PendingRequest> pending;
    {
      std::unique_lock<std::mutex> lock(g_queue_mutex);
      g_queue_cv.wait(lock, [] { return g_stopping.load() || !g_request_queue.empty(); });
      if (g_stopping.load() && g_request_queue.empty()) {
        return;
      }
      pending = g_request_queue.front();
      g_request_queue.pop_front();
    }

    compile_service::CompileServiceResponse response;
    auto wait_time = std::chrono::steady_clock::now() - pending->enqueued_at;
    if (wait_time > kQueueTimeout) {
      if (pending->request.op == "dev_test") {
        response =
            compile_service::build_dev_test_queue_timeout_response(pending->request.target, 5000);
      } else {
        response = compile_service::build_compile_queue_timeout_response(
            compile_request_kind_from_target(pending->request.target), pending->request.target, 5000);
      }
    } else {
      compile_service::RequestExecutor executor;
      {
        std::lock_guard<std::mutex> lock(g_queue_mutex);
        executor = g_request_executor;
      }
      response = executor(pending->request);
    }

    {
      std::lock_guard<std::mutex> lock(g_queue_mutex);
      pending->response = std::move(response);
      pending->completed = true;
    }
    pending->done_cv.notify_one();
  }
}

void service_loop() {
  debug_message("compile_service: service loop starting for %s\n", g_pipe_name.c_str());
  while (!g_stopping.load()) {
    HANDLE pipe =
        CreateNamedPipeA(g_pipe_name.c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                         PIPE_UNLIMITED_INSTANCES, 64 * 1024, 64 * 1024, 0, nullptr);
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

}  // namespace

bool start_compile_service(std::string_view config_path) {
  if (g_running.load()) {
    return true;
  }

  g_service_id = compile_service::make_compile_service_id(config_path);
  g_pipe_name = compile_service::make_compile_service_pipe_name(g_service_id);
  debug_message("compile_service: starting id=%s pipe=%s\n", g_service_id.c_str(), g_pipe_name.c_str());
  g_stopping = false;
  g_worker_thread = std::thread(request_worker_loop);
  g_server_thread = std::thread(service_loop);
  g_running = true;
  return true;
}

void stop_compile_service() {
  if (!g_running.load()) {
    return;
  }

  g_stopping = true;
  g_queue_cv.notify_all();
  HANDLE pipe =
      CreateFileA(g_pipe_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
  if (pipe != INVALID_HANDLE_VALUE) {
    CloseHandle(pipe);
  }

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
  if (g_worker_thread.joinable()) {
    g_worker_thread.join();
  }
  g_running = false;
}

bool compile_service_running() { return g_running.load(); }

std::string compile_service_id() { return g_service_id; }

namespace compile_service {

CompileServiceResponse dispatch_compile_service_request_for_testing(const CompileServiceRequest &request) {
  return ::dispatch_compile_service_request(request);
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
