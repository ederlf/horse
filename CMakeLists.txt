cmake_minimum_required(VERSION 3.20)
project(
    horse
    VERSION 0.1.0
    LANGUAGES C)

include(GNUInstallDirs)
include(CTest)
include(FetchContent)
option(HORSE_BUILD_PYTHON "Build Python bindings" ${SKBUILD})
option(HORSE_WARNINGS_AS_ERRORS "Treat Horse compiler warnings as errors" OFF)
option(HORSE_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(EVENT REQUIRED IMPORTED_TARGET libevent libevent_pthreads)
pkg_check_modules(PCAP REQUIRED IMPORTED_TARGET libpcap)

# Also honor the established Python packaging environment overrides.
foreach(dependency IN ITEMS CFLUID CMOCKERY)
    if(NOT DEFINED FETCHCONTENT_SOURCE_DIR_${dependency}
       AND DEFINED ENV{FETCHCONTENT_SOURCE_DIR_${dependency}})
        set(FETCHCONTENT_SOURCE_DIR_${dependency}
            "$ENV{FETCHCONTENT_SOURCE_DIR_${dependency}}"
            CACHE PATH "Local dependency checkout")
    endif()
endforeach()
FetchContent_Declare(
    cfluid
    GIT_REPOSITORY https://github.com/ederlf/libcfluid_base.git
    GIT_TAG main
    SOURCE_SUBDIR horse-unused)
FetchContent_MakeAvailable(cfluid)

file(GLOB LOCI_SOURCES CONFIGURE_DEPENDS vendor/loci/src/*.c)
add_library(loci STATIC ${LOCI_SOURCES})
target_include_directories(
    loci SYSTEM
    PUBLIC vendor/loci
    PRIVATE vendor)
add_library(
    cfluid STATIC
    ${cfluid_SOURCE_DIR}/cfluid/of_client.c
    ${cfluid_SOURCE_DIR}/cfluid/of_conn.c
    ${cfluid_SOURCE_DIR}/cfluid/of_settings.c
    ${cfluid_SOURCE_DIR}/cfluid/base/base_of_client.c
    ${cfluid_SOURCE_DIR}/cfluid/base/base_of_conn.c
    ${cfluid_SOURCE_DIR}/cfluid/base/evloop.c
    ${cfluid_SOURCE_DIR}/cfluid/base/timed_callback.c)
target_include_directories(cfluid SYSTEM PUBLIC ${cfluid_SOURCE_DIR} vendor)
target_compile_definitions(cfluid PRIVATE _GNU_SOURCE)
# The pinned fork omits the declaration of evthread_use_pthreads.
target_compile_options(cfluid PRIVATE -include event2/thread.h)
target_link_libraries(cfluid PUBLIC PkgConfig::EVENT Threads::Threads)
add_library(horse_vendor STATIC vendor/json/json.c vendor/log/log.c
                                vendor/patricia/patricia.c vendor/netemu/netns.c)
target_include_directories(horse_vendor SYSTEM PUBLIC vendor)
target_compile_definitions(horse_vendor PRIVATE _GNU_SOURCE)

add_library(horse_options INTERFACE)
target_compile_features(horse_options INTERFACE c_std_11)
target_compile_options(
    horse_options INTERFACE -Wall -Wextra -Wstrict-prototypes
                            $<$<BOOL:${HORSE_WARNINGS_AS_ERRORS}>:-Werror>)
target_compile_definitions(horse_options INTERFACE _GNU_SOURCE)
if(HORSE_SANITIZERS)
    target_compile_options(horse_options INTERFACE -fsanitize=address,undefined
                                                   -fno-omit-frame-pointer)
    target_link_options(horse_options INTERFACE -fsanitize=address,undefined)
endif()

include(cmake/Sources.cmake)
add_library(horse SHARED ${HORSE_SOURCES})
target_include_directories(horse PUBLIC src)
target_link_libraries(
    horse
    PUBLIC loci cfluid horse_vendor
    PRIVATE horse_options m)
set_target_properties(horse PROPERTIES INSTALL_RPATH "")

add_executable(horse_bin src/main.c)
set_target_properties(
    horse_bin PROPERTIES OUTPUT_NAME horse INSTALL_RPATH
                                           "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
target_link_libraries(horse_bin PRIVATE horse horse_options)
add_executable(horse_daemon src/horse_daemon.c)
set_target_properties(horse_daemon PROPERTIES INSTALL_RPATH
                                              "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
target_link_libraries(horse_daemon PRIVATE horse horse_options PkgConfig::PCAP)
install(
    TARGETS horse horse_bin horse_daemon
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT native
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT native)

if(HORSE_BUILD_PYTHON)
    include(cmake/Python.cmake)
endif()
if(BUILD_TESTING)
    FetchContent_Declare(
        cmockery
        GIT_REPOSITORY https://github.com/google/cmockery.git
        GIT_TAG 399faa2654b7da1940bf6c0d74f14468b7059512
        SOURCE_SUBDIR horse-unused)
    FetchContent_MakeAvailable(cmockery)
    add_library(cmockery STATIC ${cmockery_SOURCE_DIR}/src/cmockery.c)
    target_include_directories(
        cmockery SYSTEM
        PUBLIC ${cmockery_SOURCE_DIR}/src
        PRIVATE ${cmockery_SOURCE_DIR}/src/google)
    foreach(test_name flow_table_test scheduler_test instruction_test netflow_test)
        add_executable(${test_name} tests/${test_name}.c)
        target_compile_definitions(${test_name} PRIVATE UNIT_TESTING=1)
        target_link_libraries(${test_name} PRIVATE horse cmockery horse_options)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()
