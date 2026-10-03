cmake_minimum_required(VERSION 3.20)
project(horse VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
include(GNUInstallDirs)
include(CTest)
include(FetchContent)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(EVENT REQUIRED IMPORTED_TARGET libevent libevent_pthreads)
pkg_check_modules(PCAP REQUIRED IMPORTED_TARGET libpcap)

# Source overrides for offline builds: FETCHCONTENT_SOURCE_DIR_CFLUID/CMOCKERY.
FetchContent_Declare(cfluid
    GIT_REPOSITORY https://github.com/ederlf/libcfluid_base.git
    GIT_TAG 4146f44e119c50eb0c29d0cd253436768c9da5a7
    SOURCE_SUBDIR horse-unused)
FetchContent_MakeAvailable(cfluid)

file(GLOB LOCI_SOURCES CONFIGURE_DEPENDS vendor/loci/src/*.c)
add_library(loci STATIC ${LOCI_SOURCES})
target_include_directories(loci PUBLIC vendor/loci PRIVATE vendor)

add_library(cfluid STATIC
    ${cfluid_SOURCE_DIR}/cfluid/of_client.c
    ${cfluid_SOURCE_DIR}/cfluid/of_conn.c
    ${cfluid_SOURCE_DIR}/cfluid/of_settings.c
    ${cfluid_SOURCE_DIR}/cfluid/base/base_of_client.c
    ${cfluid_SOURCE_DIR}/cfluid/base/base_of_conn.c
    ${cfluid_SOURCE_DIR}/cfluid/base/evloop.c
    ${cfluid_SOURCE_DIR}/cfluid/base/timed_callback.c)
target_include_directories(cfluid PUBLIC ${cfluid_SOURCE_DIR} vendor)
target_compile_definitions(cfluid PRIVATE _GNU_SOURCE)
# This pinned fork calls evthread_use_pthreads without including its header.
target_compile_options(cfluid PRIVATE -include event2/thread.h)
target_link_libraries(cfluid PUBLIC PkgConfig::EVENT Threads::Threads)

file(GLOB_RECURSE HORSE_SOURCES CONFIGURE_DEPENDS
    src/lib/*.c src/net/*.c src/sim/*.c src/routing_msg/*.c)
add_library(horse SHARED ${HORSE_SOURCES}
    vendor/json/json.c vendor/log/log.c vendor/patricia/patricia.c
    vendor/netemu/netns.c)
target_include_directories(horse PUBLIC src vendor vendor/loci ${cfluid_SOURCE_DIR})
target_compile_definitions(horse PRIVATE _GNU_SOURCE)
target_compile_options(horse PRIVATE -Wall -Wextra -Wno-unused-parameter)
target_link_libraries(horse PRIVATE loci cfluid PkgConfig::EVENT Threads::Threads m)
# The only runtime libraries are system libraries; no checkout-specific RPATH.
set_target_properties(horse PROPERTIES INSTALL_RPATH "")

add_executable(horse_bin src/main.c)
set_target_properties(horse_bin PROPERTIES OUTPUT_NAME horse
    INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
target_link_libraries(horse_bin PRIVATE horse)
add_executable(horse_daemon src/horse_daemon.c)
set_target_properties(horse_daemon PROPERTIES
    INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
target_compile_definitions(horse_daemon PRIVATE _GNU_SOURCE)
target_link_libraries(horse_daemon PRIVATE horse PkgConfig::PCAP)
install(TARGETS horse horse_bin horse_daemon
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(BUILD_TESTING)
    FetchContent_Declare(cmockery
        GIT_REPOSITORY https://github.com/google/cmockery.git
        GIT_TAG 399faa2654b7da1940bf6c0d74f14468b7059512
        SOURCE_SUBDIR horse-unused)
    FetchContent_MakeAvailable(cmockery)
    add_library(cmockery STATIC ${cmockery_SOURCE_DIR}/src/cmockery.c)
    target_include_directories(cmockery PUBLIC ${cmockery_SOURCE_DIR}/src PRIVATE ${cmockery_SOURCE_DIR}/src/google)
    foreach(test_name flow_table_test scheduler_test instruction_test netflow_test)
        add_executable(${test_name} tests/${test_name}.c)
        target_compile_definitions(${test_name} PRIVATE UNIT_TESTING=1)
        target_link_libraries(${test_name} PRIVATE horse cmockery)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()
