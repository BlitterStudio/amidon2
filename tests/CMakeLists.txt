cmake_minimum_required(VERSION 3.14)
project(Amidon2Tests C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

include(CheckTypeSize)
check_type_size("long" SIZEOF_LONG)
check_type_size("int" SIZEOF_INT)
check_type_size("long long" SIZEOF_LONG_LONG)
check_type_size("size_t" SIZEOF_SIZE_T)
check_type_size("ssize_t" SIZEOF_SSIZE_T)
check_type_size("int64_t" SIZEOF_INT64_T)

include(CheckIncludeFile)
check_include_file("inttypes.h" HAVE_INTTYPES_H)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/host_config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/config.h
)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/json-c)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/json-c/json.h
    "#ifndef _json_h_\n"
    "#define _json_h_\n"
    "#ifdef __cplusplus\n"
    "extern \"C\" {\n"
    "#endif\n"
    "#include <arraylist.h>\n"
    "#include <debug.h>\n"
    "#include <json_c_version.h>\n"
    "#include <json_object.h>\n"
    "#include <json_object_iterator.h>\n"
    "#include <json_patch.h>\n"
    "#include <json_pointer.h>\n"
    "#include <json_tokener.h>\n"
    "#include <json_util.h>\n"
    "#include <linkhash.h>\n"
    "#include <json_visit.h>\n"
    "#ifdef __cplusplus\n"
    "}\n"
    "#endif\n"
    "#endif\n"
)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/xlocale.h
    "#include <locale.h>\n"
)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PROJECT_ROOT}/src
    ${PROJECT_ROOT}/vendor/json-c
    ${PROJECT_ROOT}/vendor/json-c/include/json-c
)

set(JSON_C_SOURCES
    ${PROJECT_ROOT}/vendor/json-c/arraylist.c
    ${PROJECT_ROOT}/vendor/json-c/debug.c
    ${PROJECT_ROOT}/vendor/json-c/json_c_version.c
    ${PROJECT_ROOT}/vendor/json-c/json_object.c
    ${PROJECT_ROOT}/vendor/json-c/json_object_iterator.c
    ${PROJECT_ROOT}/vendor/json-c/json_patch.c
    ${PROJECT_ROOT}/vendor/json-c/json_pointer.c
    ${PROJECT_ROOT}/vendor/json-c/json_tokener.c
    ${PROJECT_ROOT}/vendor/json-c/json_util.c
    ${PROJECT_ROOT}/vendor/json-c/json_visit.c
    ${PROJECT_ROOT}/vendor/json-c/linkhash.c
    ${PROJECT_ROOT}/vendor/json-c/printbuf.c
    ${PROJECT_ROOT}/vendor/json-c/random_seed.c
    ${PROJECT_ROOT}/vendor/json-c/strerror_override.c
)

set(LOGIC_SOURCES
    ${PROJECT_ROOT}/src/logic/TimelineParser.cpp
    ${PROJECT_ROOT}/src/logic/AccountParser.cpp
    ${PROJECT_ROOT}/src/logic/SettingsParser.cpp
    ${PROJECT_ROOT}/src/logic/TootFormatter.cpp
    ${PROJECT_ROOT}/src/logic/UrlBuilder.cpp
    ${PROJECT_ROOT}/src/logic/CredsParser.cpp
)

set(TEST_SOURCES
    main.cpp
    test_settings.cpp
    test_timeline.cpp
    test_account.cpp
    test_toot_format.cpp
    test_url_builder.cpp
    test_creds.cpp
)

add_executable(amidon2_tests ${TEST_SOURCES} ${LOGIC_SOURCES} ${JSON_C_SOURCES})

if(NOT WIN32)
    target_link_libraries(amidon2_tests PRIVATE m)
endif()
