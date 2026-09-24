cmake_minimum_required(VERSION 3.24)

project(
  base
  VERSION 0.1.0
  DESCRIPTION "BASE block library"
  LANGUAGES CXX
)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

option(BASE_BUILD_TESTS "Build tests" ON)

set(BASE_PUBLIC_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/include/bld.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/base.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/wasm_host.hpp
)

add_library(base INTERFACE)
add_library(base::base ALIAS base)

target_compile_features(base INTERFACE cxx_std_23)
target_sources(
  base
  INTERFACE
    FILE_SET HEADERS
    BASE_DIRS
      ${CMAKE_CURRENT_SOURCE_DIR}/include
      ${CMAKE_CURRENT_SOURCE_DIR}/src
    FILES ${BASE_PUBLIC_HEADERS}
)

install(
  TARGETS base
  FILE_SET HEADERS
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(BASE_BUILD_TESTS)
  include(FetchContent)

  set(DOCTEST_WITH_TESTS OFF CACHE BOOL "" FORCE)
  set(DOCTEST_WITH_MAIN_IN_STATIC_LIB OFF CACHE BOOL "" FORCE)
  set(DOCTEST_NO_INSTALL ON CACHE BOOL "" FORCE)

  FetchContent_Declare(
    doctest
    URL https://github.com/doctest/doctest/archive/refs/tags/v2.5.3.tar.gz
    URL_HASH SHA256=174ebc4e769928959614789c5b4e9c3d0a0f81a62bb608756b127bfebfb21331
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_MakeAvailable(doctest)

  add_executable(base_tests
    tests/mock_runtime.cpp
    tests/unit/test_blocks.cpp
  )
  target_link_libraries(base_tests PRIVATE base doctest::doctest)
  target_compile_definitions(base_tests PRIVATE DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN)

  # wasm_host.hpp is an alternate host and is not linked into the tests.
  set(base_wasm_host_tu "${CMAKE_CURRENT_BINARY_DIR}/wasm_host_tu.cpp")
  file(WRITE "${base_wasm_host_tu}" [[
#include <wasm_host.hpp>

extern "C" void mount() {}
extern "C" void host_sendPinF32(u32, u8, f32) {}
]])
  add_library(base_wasm_host OBJECT "${base_wasm_host_tu}")
  target_link_libraries(base_wasm_host PRIVATE base)

  include(CTest)
  add_test(NAME base_tests COMMAND base_tests)
endif()
