#include "base/std.h"
#include "compile_service.h"

#include <atomic>
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
std::string g_service_id;
std::string g_pipe_name;
std::atomic<unsigned long> g_last_pipe_error{0};

void service_loop() {
  debug_message("compile_service: service loop starting for %s\n", g_pipe_name.c_str());
  while (!g_stopping.load()) {
    HANDLE pipe =
        CreateNamedPipeA(g_pipe_name.c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                         1, 64 * 1024, 64 * 1024, 0, nullptr);
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
      auto response = compile_service::execute_compile_service_request(request);
      auto payload = nlohmann::json(response).dump() + "\n";
      DWORD bytes_written = 0;
      WriteFile(pipe, payload.data(), static_cast<DWORD>(payload.size()), &bytes_written, nullptr);
      FlushFileBuffers(pipe);
    }

    DisconnectNamedPipe(pipe);
    CloseHandle(pipe);
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
  g_server_thread = std::thread(service_loop);
  g_running = true;
  return true;
}

void stop_compile_service() {
  if (!g_running.load()) {
    return;
  }

  g_stopping = true;
  HANDLE pipe =
      CreateFileA(g_pipe_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
  if (pipe != INVALID_HANDLE_VALUE) {
    CloseHandle(pipe);
  }

  if (g_server_thread.joinable()) {
    g_server_thread.join();
  }
  g_running = false;
}

bool compile_service_running() { return g_running.load(); }

std::string compile_service_id() { return g_service_id; }
