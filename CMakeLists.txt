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

add_library(base INTERFACE)
add_library(base::base ALIAS base)

target_compile_features(base INTERFACE cxx_std_23)
target_include_directories(
  base
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY core/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/core FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY blocks/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/blocks FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY math/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/math FILES_MATCHING PATTERN "*.hpp")

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
    tests/unit/test_blocks_f64.cpp
  )
  target_link_libraries(base_tests PRIVATE base doctest::doctest)
  set_source_files_properties(tests/unit/test_blocks.cpp PROPERTIES COMPILE_DEFINITIONS DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN)

  # Platform hosts are alternate C ABIs and are not linked into the tests.
  add_library(base_wasm_host OBJECT arch/wasm/host.cpp)
  target_link_libraries(base_wasm_host PRIVATE base)

  add_library(base_mcu_hal OBJECT arch/mcu/hal.cpp)
  target_link_libraries(base_mcu_hal PRIVATE base)

  include(CTest)
  add_test(NAME base_tests COMMAND base_tests)
endif()
