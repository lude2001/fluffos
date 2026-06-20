#ifndef COMPILE_SERVICE_H
#define COMPILE_SERVICE_H

#include <functional>
#include <string>
#include <string_view>

#include "compile_service_protocol.h"

bool start_compile_service(std::string_view config_path);
void stop_compile_service();
bool compile_service_running();
std::string compile_service_id();

namespace compile_service {

using RequestExecutor = std::function<CompileServiceResponse(const CompileServiceRequest &)>;

CompileServiceResponse dispatch_compile_service_request_for_testing(const CompileServiceRequest &request);
int process_compile_service_requests_on_main_thread(int max_requests = 1);
int process_compile_service_requests_for_testing(int max_requests = 1);
void set_compile_service_request_executor_for_testing(RequestExecutor executor);
void clear_compile_service_request_executor_for_testing();

}  // namespace compile_service

#endif  // COMPILE_SERVICE_H
