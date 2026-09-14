option(PACKAGE_JSON "native JSON efun package" ON)

if(PACKAGE_JSON)
    add_library(package_json STATIC
            "json.cc"
            )
endif()
