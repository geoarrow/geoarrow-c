
message(STATUS "Building using CMake version: ${CMAKE_VERSION}")
cmake_minimum_required(VERSION 3.11)

project(GeoArrow)


option(GEOARROW_CODE_COVERAGE "Enable coverage reporting" OFF)
add_library(coverage_config INTERFACE)

include_directories(src)
add_library(
  geoarrow
  src/geoarrow/schema.c
  src/geoarrow/schema_view.c
  src/geoarrow/metadata.c
  src/geoarrow/visitor.c
  src/geoarrow/wkt_reader.c
  src/geoarrow/wkt_writer.c
  src/geoarrow/nanoarrow.c)

if(GEOARROW_CODE_COVERAGE)
  target_compile_options(coverage_config INTERFACE -O0 -g --coverage)
  target_link_options(coverage_config INTERFACE --coverage)
endif()


if (GEOARROW_BUILD_TESTS)

  set(MEMORYCHECK_COMMAND_OPTIONS "--leak-check=full --suppressions=${CMAKE_CURRENT_LIST_DIR}/valgrind.supp --error-exitcode=1")
  include(CTest)
  include(FetchContent)

  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)

  find_package(Arrow REQUIRED)

  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/release-1.11.0.zip
  )
  FetchContent_MakeAvailable(googletest)

  enable_testing()

  add_executable(schema_test src/geoarrow/schema_test.cc)
  add_executable(schema_view_test src/geoarrow/schema_view_test.cc)
  add_executable(metadata_test src/geoarrow/metadata_test.cc)
  add_executable(visitor_test src/geoarrow/visitor_test.cc)
  add_executable(wkt_reader_test src/geoarrow/wkt_reader_test.cc)
  add_executable(wkt_writer_test src/geoarrow/wkt_writer_test.cc)
  add_executable(geoarrow_arrow_test src/geoarrow/geoarrow_arrow_test.cc)

  if(GEOARROW_CODE_COVERAGE)
      target_compile_options(coverage_config INTERFACE -O0 -g --coverage)
      target_link_options(coverage_config INTERFACE --coverage)
      target_link_libraries(geoarrow coverage_config)
  endif()


  target_link_libraries(schema_test geoarrow arrow_shared gtest_main)
  target_link_libraries(schema_view_test geoarrow gtest_main)
  target_link_libraries(metadata_test geoarrow gtest_main)
  target_link_libraries(visitor_test geoarrow gtest_main)
  target_link_libraries(wkt_reader_test geoarrow gtest_main)
  target_link_libraries(wkt_writer_test geoarrow gtest_main)
  target_link_libraries(geoarrow_arrow_test geoarrow arrow_shared gtest_main)

  include(GoogleTest)
  gtest_discover_tests(schema_test)
  gtest_discover_tests(schema_view_test)
  gtest_discover_tests(metadata_test)
  gtest_discover_tests(visitor_test)
  gtest_discover_tests(wkt_reader_test)
  gtest_discover_tests(wkt_writer_test)
  gtest_discover_tests(geoarrow_arrow_test)
endif()
