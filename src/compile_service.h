#ifndef COMPILE_SERVICE_H
#define COMPILE_SERVICE_H

#include <string>
#include <string_view>

bool start_compile_service(std::string_view config_path);
void stop_compile_service();
bool compile_service_running();
std::string compile_service_id();

#endif  // COMPILE_SERVICE_H
