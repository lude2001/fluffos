#ifndef RUNTIME_DEV_TEST_REQUEST_H
#define RUNTIME_DEV_TEST_REQUEST_H

#include "compile_service_protocol.h"

namespace compile_service {

CompileServiceResponse execute_dev_test_request(const CompileServiceRequest &request);

}

#endif  // RUNTIME_DEV_TEST_REQUEST_H
