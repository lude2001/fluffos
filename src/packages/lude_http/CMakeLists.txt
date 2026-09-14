option(PACKAGE_LUDE_HTTP "HTTP helper and streaming parser efun package" ON)

if(PACKAGE_LUDE_HTTP)
    add_library(package_lude_http STATIC
            "http_efuns.cc"
            "http_efuns.h"
            "http_parser.cc"
            "http_parser.h"
            )
endif()
