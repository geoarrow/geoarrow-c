
message(STATUS "Building using CMake version: ${CMAKE_VERSION}")
cmake_minimum_required(VERSION 3.14)

if(NOT DEFINED CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

set(GEOARROW_VERSION "0.1.0-SNAPSHOT")
string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+" GEOARROW_BASE_VERSION "${GEOARROW_VERSION}")
project(geoarrow VERSION "${GEOARROW_BASE_VERSION}")

set(GEOARROW_VERSION_MAJOR "${geoarrow_VERSION_MAJOR}")
set(GEOARROW_VERSION_MINOR "${geoarrow_VERSION_MINOR}")
set(GEOARROW_VERSION_PATCH "${geoarrow_VERSION_PATCH}")

option(GEOARROW_BUILD_TESTS "Build tests" OFF)
option(GEOARROW_CODE_COVERAGE "Enable coverage reporting" OFF)
option(GEOARROW_USE_FAST_FLOAT "Use fast_float for numeric value parsing" ON)

add_library(coverage_config INTERFACE)

configure_file(src/geoarrow/geoarrow_config.h.in generated/geoarrow_config.h)

if(GEOARROW_CODE_COVERAGE)
  target_compile_options(coverage_config INTERFACE -O0 -g --coverage)
  target_link_options(coverage_config INTERFACE --coverage)
endif()

if(GEOARROW_USE_FAST_FLOAT)
  if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 11)
  endif()
  set(GEOARROW_VALUE_PARSING_SOURCES src/geoarrow/double_serde_fast_float.cc)
else()
  set(GEOARROW_VALUE_PARSING_SOURCES src/geoarrow/double_serde_std.c)
endif()

add_library(
  geoarrow
  src/geoarrow/schema.c
  src/geoarrow/schema_view.c
  src/geoarrow/metadata.c
  src/geoarrow/kernel.c
  src/geoarrow/builder.c
  src/geoarrow/array_view.c
  src/geoarrow/util.c
  src/geoarrow/visitor.c
  src/geoarrow/wkb_reader.c
  src/geoarrow/wkb_writer.c
  src/geoarrow/wkt_reader.c
  src/geoarrow/wkt_writer.c
  ${GEOARROW_VALUE_PARSING_SOURCES}
  src/geoarrow/nanoarrow.c)

target_include_directories(geoarrow PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:include>)
target_include_directories(geoarrow
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
)
target_compile_definitions(geoarrow PUBLIC
  "$<$<CONFIG:Debug>:NANOARROW_DEBUG>"
  "$<$<CONFIG:Debug>:GEOARROW_DEBUG>")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(
      geoarrow
      PRIVATE
      -Wall
      -Werror
      -Wextra
      -Wno-type-limits
      -Wno-unused-parameter
      -Wpedantic
      -Wunused-result)
  elseif(CMAKE_C_COMPILER_ID STREQUAL "AppleClang" OR
    CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(
      geoarrow
      PRIVATE
      -Wall
      -Werror
      -Wextra
      -Wdocumentation
      -Wno-unused-parameter
      -Wshorten-64-to-32)
  endif()
endif()

install(TARGETS geoarrow DESTINATION lib)
install(DIRECTORY src/ DESTINATION include FILES_MATCHING PATTERN "*.h")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/generated/geoarrow_config.h
        DESTINATION include/geoarrow)

if(GEOARROW_BUILD_TESTS)
  set(MEMORYCHECK_COMMAND_OPTIONS "--leak-check=full --suppressions=${CMAKE_CURRENT_LIST_DIR}/valgrind.supp --error-exitcode=1")
  include(CTest)
  include(FetchContent)

  find_package(Arrow REQUIRED)

  if(${ARROW_VERSION} VERSION_GREATER_EQUAL "10.0.0")
    set(CMAKE_CXX_STANDARD 17)
  else()
    set(CMAKE_CXX_STANDARD 11)
  endif()

  set(CMAKE_CXX_STANDARD_REQUIRED ON)

  if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.23")
    cmake_policy(SET CMP0135 NEW)
  endif()

  if(MSVC)
    set(gtest_force_shared_crt on)
    set(NANOARROW_ARROW_TARGET arrow_static)
  else()
    set(NANOARROW_ARROW_TARGET arrow_shared)
  endif()

  # Use an old version of googletest if we have to to support gcc 4.8
  if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
    CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "5.0.0")
    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/release-1.11.0.zip
      URL_HASH SHA256=353571c2440176ded91c2de6d6cd88ddd41401d14692ec1f99e35d013feda55a
    )
  else()
    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/release-1.10.0.zip
      URL_HASH SHA256=94c634d499558a76fa649edb13721dce6e98fb1e7018dfaeba3cd7a083945e91
    )
  endif()

  FetchContent_MakeAvailable(googletest)

  enable_testing()

  add_executable(geoarrow_hpp_test src/geoarrow/geoarrow_hpp_test.cc)
  add_executable(geoarrow_type_inline_test src/geoarrow/geoarrow_type_inline_test.cc)
  add_executable(builder_test src/geoarrow/builder_test.cc)
  add_executable(array_view_test src/geoarrow/array_view_test.cc)
  add_executable(schema_test src/geoarrow/schema_test.cc)
  add_executable(schema_view_test src/geoarrow/schema_view_test.cc)
  add_executable(metadata_test src/geoarrow/metadata_test.cc)
  add_executable(kernel_test src/geoarrow/kernel_test.cc)
  add_executable(visitor_test src/geoarrow/visitor_test.cc)
  add_executable(util_test src/geoarrow/util_test.cc)
  add_executable(wkb_reader_test src/geoarrow/wkb_reader_test.cc)
  add_executable(wkb_writer_test src/geoarrow/wkb_writer_test.cc)
  add_executable(wkt_reader_test src/geoarrow/wkt_reader_test.cc)
  add_executable(wkt_writer_test src/geoarrow/wkt_writer_test.cc)
  add_executable(wkx_files_test src/geoarrow/wkx_files_test.cc)
  add_executable(geoarrow_arrow_test src/geoarrow/geoarrow_arrow_test.cc)

  if(GEOARROW_CODE_COVERAGE)
    target_compile_options(coverage_config INTERFACE -O0 -g --coverage)
    target_link_options(coverage_config INTERFACE --coverage)
    target_link_libraries(geoarrow coverage_config)
  endif()

  target_link_libraries(geoarrow_hpp_test geoarrow gtest_main)
  target_link_libraries(geoarrow_type_inline_test geoarrow gtest_main)
  target_link_libraries(builder_test geoarrow gtest_main)
  target_link_libraries(array_view_test geoarrow gtest_main)
  target_link_libraries(schema_test geoarrow arrow_shared gtest_main)
  target_link_libraries(schema_view_test geoarrow gtest_main)
  target_link_libraries(kernel_test geoarrow gtest_main)
  target_link_libraries(metadata_test geoarrow gtest_main)
  target_link_libraries(visitor_test geoarrow gtest_main)
  target_link_libraries(util_test geoarrow gtest_main)
  target_link_libraries(wkb_reader_test geoarrow gtest_main)
  target_link_libraries(wkb_writer_test geoarrow gtest_main)
  target_link_libraries(wkt_reader_test geoarrow gtest_main)
  target_link_libraries(wkt_writer_test geoarrow gtest_main)
  target_link_libraries(wkx_files_test geoarrow gtest_main)
  target_link_libraries(geoarrow_arrow_test geoarrow arrow_shared gtest_main)

  include(GoogleTest)
  gtest_discover_tests(geoarrow_hpp_test)
  gtest_discover_tests(geoarrow_type_inline_test)
  gtest_discover_tests(builder_test)
  gtest_discover_tests(array_view_test)
  gtest_discover_tests(schema_test)
  gtest_discover_tests(schema_view_test)
  gtest_discover_tests(kernel_test)
  gtest_discover_tests(metadata_test)
  gtest_discover_tests(visitor_test)
  gtest_discover_tests(util_test)
  gtest_discover_tests(wkb_reader_test)
  gtest_discover_tests(wkb_writer_test)
  gtest_discover_tests(wkt_reader_test)
  gtest_discover_tests(wkt_writer_test)
  gtest_discover_tests(wkx_files_test)
  gtest_discover_tests(geoarrow_arrow_test)
endif()
