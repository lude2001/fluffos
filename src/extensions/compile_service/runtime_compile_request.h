#ifndef RUNTIME_COMPILE_REQUEST_H
#define RUNTIME_COMPILE_REQUEST_H

#include "extensions/compile_service/compile_service_protocol.h"

namespace compile_service {

CompileServiceResponse execute_compile_service_request(const CompileServiceRequest &request);

}

#endif  // RUNTIME_COMPILE_REQUEST_H
