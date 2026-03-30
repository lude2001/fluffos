#include "runtime_compile_request.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

#include "compiler/internal/compiler.h"
#include "vm/internal/master.h"
#include "vm/internal/simulate.h"
#include "vm/internal/simul_efun.h"
#include "vm/internal/base/machine.h"
#include "vm/vm.h"

namespace compile_service {
namespace {

std::string normalize_lpc_target(std::string target) {
  if (target.empty()) {
    return target;
  }

  std::replace(target.begin(), target.end(), '\\', '/');
  if (target[0] != '/') {
    target.insert(target.begin(), '/');
  }
  return target;
}

std::filesystem::path target_to_filesystem_path(const std::string &target) {
  auto normalized = normalize_lpc_target(target);
  if (!normalized.empty() && normalized[0] == '/') {
    normalized.erase(normalized.begin());
  }
  return std::filesystem::path(normalized);
}

CompileServiceResponse compile_single_file(const std::string &target) {
  CompileServiceResponse response;
  response.version = 1;
  response.kind = "file";
  response.target = normalize_lpc_target(target);

  std::vector<CompileServiceDiagnostic> diagnostics;
  set_compile_service_diagnostics_collector(&diagnostics);

  object_t *result = nullptr;
  object_t *previous_object = current_object;
  error_context_t econ{};
  save_context(&econ);
  try {
    current_object = master_ob;
    if (auto *existing = find_object2(response.target.c_str())) {
      const bool handled_by_vital_reload = (existing == master_ob || existing == simul_efun_ob);
      destruct_object(existing);
      remove_destructed_objects();
      if (handled_by_vital_reload) {
        current_object = master_ob;
        result = find_object2(response.target.c_str());
      } else {
        result = load_object(response.target.c_str(), 1);
      }
    } else {
      result = load_object(response.target.c_str(), 1);
    }
  } catch (...) {
    restore_context(&econ);
  }
  pop_context(&econ);
  current_object = previous_object;

  set_compile_service_diagnostics_collector(nullptr);
  response.diagnostics = diagnostics;
  response.ok = (result != nullptr);
  return response;
}

}  // namespace

CompileServiceResponse execute_compile_service_request(const CompileServiceRequest &request) {
  auto normalized_target = normalize_lpc_target(request.target);
  auto fs_target = target_to_filesystem_path(normalized_target);

  std::error_code ec;
  if (std::filesystem::is_directory(fs_target, ec)) {
    CompileServiceResponse response;
    response.version = 1;
    response.kind = "directory";
    response.target = normalized_target;

    std::vector<std::filesystem::path> files;
    for (auto const &entry : std::filesystem::recursive_directory_iterator(fs_target, ec)) {
      if (ec) {
        break;
      }
      if (!entry.is_regular_file()) {
        continue;
      }
      if (entry.path().extension() == ".c") {
        files.push_back(entry.path());
      }
    }
    std::sort(files.begin(), files.end());

    for (auto const &file : files) {
      auto relative = std::filesystem::relative(file, std::filesystem::current_path(), ec);
      auto lpc_file = "/" + (ec ? file.generic_string() : relative.generic_string());
      auto file_response = compile_single_file(lpc_file);
      response.results.push_back(nlohmann::json{{"file", file_response.target},
                                                {"ok", file_response.ok},
                                                {"diagnostics", file_response.diagnostics}});
      response.files_total++;
      if (file_response.ok) {
        response.files_ok++;
      } else {
        response.files_failed++;
      }
    }
    response.ok = (response.files_failed == 0);
    return response;
  }

  return compile_single_file(normalized_target);
}

}  // namespace compile_service
