#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

namespace fs = std::filesystem;

namespace {

void print_usage() {
  std::cerr << "Usage: lpcprj <config-file> [driver-args...]\n";
}

#ifdef _WIN32
std::wstring quote_windows_argument(const std::wstring &arg) {
  if (arg.empty()) {
    return L"\"\"";
  }

  bool needs_quotes = false;
  for (wchar_t ch : arg) {
    if (iswspace(ch) || ch == L'"') {
      needs_quotes = true;
      break;
    }
  }
  if (!needs_quotes) {
    return arg;
  }

  std::wstring quoted = L"\"";
  size_t backslashes = 0;
  for (wchar_t ch : arg) {
    if (ch == L'\\') {
      backslashes++;
      continue;
    }

    if (ch == L'"') {
      quoted.append(backslashes * 2 + 1, L'\\');
      quoted.push_back(ch);
      backslashes = 0;
      continue;
    }

    if (backslashes > 0) {
      quoted.append(backslashes, L'\\');
      backslashes = 0;
    }
    quoted.push_back(ch);
  }

  if (backslashes > 0) {
    quoted.append(backslashes * 2, L'\\');
  }
  quoted.push_back(L'"');
  return quoted;
}

std::wstring format_windows_error(DWORD error_code) {
  LPWSTR message_buffer = nullptr;
  const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
  const DWORD length = FormatMessageW(flags, nullptr, error_code, 0, reinterpret_cast<LPWSTR>(&message_buffer), 0,
                                      nullptr);
  std::wstring message;
  if (length != 0 && message_buffer != nullptr) {
    message.assign(message_buffer, message_buffer + length);
    while (!message.empty() && (message.back() == L'\r' || message.back() == L'\n')) {
      message.pop_back();
    }
  } else {
    message = L"Unknown error";
  }
  if (message_buffer != nullptr) {
    LocalFree(message_buffer);
  }
  return message;
}

std::vector<std::wstring> get_command_line_args() {
  int argc = 0;
  LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  if (argv == nullptr) {
    return {};
  }

  std::vector<std::wstring> args;
  args.reserve(static_cast<size_t>(argc));
  for (int i = 0; i < argc; i++) {
    args.emplace_back(argv[i]);
  }
  LocalFree(argv);
  return args;
}

fs::path get_module_path() {
  std::wstring buffer(MAX_PATH, L'\0');
  for (;;) {
    const DWORD copied = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (copied == 0) {
      throw std::runtime_error("GetModuleFileNameW failed");
    }
    if (copied < buffer.size() - 1) {
      buffer.resize(copied);
      return fs::path(buffer);
    }
    buffer.resize(buffer.size() * 2);
  }
}

std::wstring build_path_environment(const fs::path &runtime_dir, const fs::path &driver_dir) {
  const DWORD required = GetEnvironmentVariableW(L"PATH", nullptr, 0);
  std::wstring existing;
  if (required != 0) {
    existing.resize(required - 1);
    GetEnvironmentVariableW(L"PATH", existing.data(), required);
  }

  std::wstring updated = runtime_dir.native();
  updated.push_back(L';');
  updated.append(driver_dir.native());
  if (!existing.empty()) {
    updated.push_back(L';');
    updated.append(existing);
  }
  return updated;
}
#endif

}  // namespace

int main(int argc, char ** /*argv*/) {
  if (argc < 2) {
    print_usage();
    return 1;
  }

#ifndef _WIN32
  std::cerr << "lpcprj is currently implemented for Windows only.\n";
  return 2;
#else
  std::vector<std::wstring> args = get_command_line_args();
  if (args.size() < 2) {
    print_usage();
    return 1;
  }

  try {
    const fs::path module_path = get_module_path();
    const fs::path install_root = module_path.parent_path().parent_path();
    const fs::path driver_path = install_root / "libexec" / "fluffos" / "driver.exe";
    const fs::path runtime_path = install_root / "runtime";
    const fs::path config_path = fs::absolute(fs::path(args[1])).lexically_normal();
    const fs::path config_dir = config_path.parent_path();

    if (!fs::exists(driver_path)) {
      std::wcerr << L"Unable to find driver executable at " << driver_path.native() << L"\n";
      return 2;
    }

    if (!fs::exists(config_path)) {
      std::wcerr << L"Unable to find config file at " << config_path.native() << L"\n";
      return 1;
    }

    const std::wstring updated_path = build_path_environment(runtime_path, driver_path.parent_path());
    SetEnvironmentVariableW(L"PATH", updated_path.c_str());

    std::wstring command_line = quote_windows_argument(driver_path.native());
    command_line.push_back(L' ');
    command_line.append(quote_windows_argument(config_path.native()));
    for (size_t i = 2; i < args.size(); i++) {
      command_line.push_back(L' ');
      command_line.append(quote_windows_argument(args[i]));
    }

    STARTUPINFOW startup_info{};
    startup_info.cb = sizeof(startup_info);
    PROCESS_INFORMATION process_info{};

    std::wstring mutable_command_line = command_line;
    const BOOL created = CreateProcessW(driver_path.c_str(), mutable_command_line.data(), nullptr, nullptr, FALSE, 0,
                                        nullptr, config_dir.c_str(), &startup_info, &process_info);
    if (!created) {
      const auto error_code = GetLastError();
      std::wcerr << L"Failed to launch driver.exe: " << format_windows_error(error_code) << L" (" << error_code
                 << L")\n";
      return 2;
    }

    WaitForSingleObject(process_info.hProcess, INFINITE);

    DWORD exit_code = 0;
    GetExitCodeProcess(process_info.hProcess, &exit_code);

    CloseHandle(process_info.hThread);
    CloseHandle(process_info.hProcess);

    return static_cast<int>(exit_code);
  } catch (const std::exception &ex) {
    std::cerr << "lpcprj failed: " << ex.what() << "\n";
    return 2;
  }
#endif
}
