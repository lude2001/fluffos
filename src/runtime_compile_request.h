#ifndef RUNTIME_COMPILE_REQUEST_H
#define RUNTIME_COMPILE_REQUEST_H

#include "compile_service_protocol.h"

namespace compile_service {

CompileServiceResponse execute_compile_service_request(const CompileServiceRequest &request);

}

#endif  // RUNTIME_COMPILE_REQUEST_H
