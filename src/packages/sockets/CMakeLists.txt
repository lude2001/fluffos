if(${PACKAGE_SOCKETS})
    add_library(package_sockets STATIC
            "http_efuns.cc"
            "http_efuns.h"
            "http_parser.cc"
            "http_parser.h"
            "socket_efuns.cc"
            "socket_efuns.h"
            "sockets.cc"
    )
endif()
