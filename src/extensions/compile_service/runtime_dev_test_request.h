#ifndef RUNTIME_DEV_TEST_REQUEST_H
#define RUNTIME_DEV_TEST_REQUEST_H

#include "extensions/compile_service/compile_service_protocol.h"

namespace compile_service {

CompileServiceResponse execute_dev_test_request(const CompileServiceRequest &request);

}

#endif  // RUNTIME_DEV_TEST_REQUEST_H
