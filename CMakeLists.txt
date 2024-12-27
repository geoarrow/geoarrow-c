message(STATUS "Building using CMake version: ${CMAKE_VERSION}")
cmake_minimum_required(VERSION 3.14)

if(NOT DEFINED CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

set(GEOARROW_VERSION "0.2.0-SNAPSHOT")
string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+" GEOARROW_BASE_VERSION
             "${GEOARROW_VERSION}")
project(geoarrow VERSION "${GEOARROW_BASE_VERSION}")

set(GEOARROW_VERSION_MAJOR "${geoarrow_VERSION_MAJOR}")
set(GEOARROW_VERSION_MINOR "${geoarrow_VERSION_MINOR}")
set(GEOARROW_VERSION_PATCH "${geoarrow_VERSION_PATCH}")

option(GEOARROW_BUILD_TESTS "Build tests" OFF)
option(GEOARROW_CODE_COVERAGE "Enable coverage reporting" OFF)
option(GEOARROW_USE_FAST_FLOAT "Use fast_float for numeric value parsing" ON)
option(GEOARROW_USE_RYU "Use ryu for numeric value printing" ON)
option(GEOARROW_NAMESPACE "A prefix for exported symbols" OFF)
option(GEOARROW_ARROW_STATIC "Use a statically-linked Arrow C++ build when linking tests"
       OFF)
option(GEOARROW_BUNDLE "Use ON to just bundle the headers/sources" OFF)

if(GEOARROW_NAMESPACE)
  set(GEOARROW_NAMESPACE_DEFINE "#define GEOARROW_NAMESPACE ${GEOARROW_NAMESPACE}")
else()
  set(GEOARROW_NAMESPACE_DEFINE "// #define GEOARROW_NAMESPACE YourNamespaceHere")
endif()

if(GEOARROW_BUILD_TESTS)
  add_library(coverage_config INTERFACE)
endif()

if(GEOARROW_CODE_COVERAGE)
  target_compile_options(coverage_config INTERFACE -O0 -g --coverage)
  target_link_options(coverage_config INTERFACE --coverage)
endif()

if(GEOARROW_USE_FAST_FLOAT)
  if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 11)
  endif()

  set(GEOARROW_DOUBLE_PARSE_SOURCE src/geoarrow/double_parse_fast_float.cc)
  set(GEOARROW_USE_FAST_FLOAT_DEFINE "#define GEOARROW_USE_FAST_FLOAT 1")
else()
  set(GEOARROW_DOUBLE_PARSE_SOURCE src/geoarrow/double_parse_std.c)
  set(GEOARROW_USE_FAST_FLOAT_DEFINE "#define GEOARROW_USE_FAST_FLOAT 0")
endif()

if(GEOARROW_USE_RYU)
  set(GEOARROW_DOUBLE_PRINT_SOURCE src/geoarrow/double_print.c src/geoarrow/ryu/d2s.c)
  set(GEOARROW_USE_RYU_DEFINE "#define GEOARROW_USE_RYU 1")
else()
  set(GEOARROW_DOUBLE_PRINT_SOURCE src/geoarrow/double_print.c)
  set(GEOARROW_USE_RYU_DEFINE "#define GEOARROW_USE_RYU 0")
endif()

configure_file(src/geoarrow/geoarrow_config.h.in generated/geoarrow_config.h)

if(NOT TARGET nanoarrow::nanoarrow)
  include(FetchContent)

  set(GEOARROW_NANOARROW_TARGET_VERSION "0.6.0")
  fetchcontent_declare(nanoarrow
                       URL "https://www.apache.org/dyn/closer.lua?action=download&filename=arrow/apache-arrow-nanoarrow-${GEOARROW_NANOARROW_TARGET_VERSION}/apache-arrow-nanoarrow-${GEOARROW_NANOARROW_TARGET_VERSION}.tar.gz"
                           "https://github.com/apache/arrow-nanoarrow/releases/download/apache-arrow-nanoarrow-${GEOARROW_NANOARROW_TARGET_VERSION}/apache-arrow-nanoarrow-${GEOARROW_NANOARROW_TARGET_VERSION}.tar.gz"
                       URL_HASH SHA256=e4a02ac51002ad1875bf09317e70adb959005fad52b240ff59f73b970fa485d1
  )
  fetchcontent_makeavailable(nanoarrow)
endif()

set(GEOARROW_SOURCES
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
    src/geoarrow/array_reader.c
    src/geoarrow/array_writer.c)

set(GEOARROW_PUBLIC_HEADERS
    ${CMAKE_CURRENT_BINARY_DIR}/generated/geoarrow_config.h src/geoarrow/geoarrow_type.h
    src/geoarrow/geoarrow.h src/geoarrow/geoarrow_type_inline.h)

if(GEOARROW_BUNDLE)
  # Bundle sources + headers, and if building tests, add a target the tests can link against
  file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/amalgamation)
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/amalgamation)
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/amalgamation/geoarrow)

  set(GEOARROW_H_TEMP ${CMAKE_BINARY_DIR}/amalgamation/geoarrow/geoarrow.h)
  file(WRITE ${GEOARROW_H_TEMP} "")

  foreach(SRC_FILE ${GEOARROW_PUBLIC_HEADERS})
    file(READ ${SRC_FILE} SRC_FILE_CONTENTS)
    file(APPEND ${GEOARROW_H_TEMP} "${SRC_FILE_CONTENTS}")
  endforeach()

  set(GEOARROW_C_TEMP ${CMAKE_BINARY_DIR}/amalgamation/geoarrow/geoarrow.c)

  foreach(SRC_FILE ${GEOARROW_SOURCES} src/geoarrow/double_print.c
                   src/geoarrow/double_parse_std.c)
    file(READ ${SRC_FILE} SRC_FILE_CONTENTS)
    file(APPEND ${GEOARROW_C_TEMP} "${SRC_FILE_CONTENTS}")
  endforeach()

  # Update includes for concatenated sources
  foreach(SRC_FILE ${GEOARROW_H_TEMP} ${GEOARROW_C_TEMP})
    file(READ ${SRC_FILE} SRC_FILE_CONTENTS)
    string(REGEX REPLACE "#include \"[a-z_.]+\"" "" SRC_FILE_CONTENTS
                         "${SRC_FILE_CONTENTS}")
    file(WRITE ${SRC_FILE} "${SRC_FILE_CONTENTS}")
  endforeach()

  # Make sure geoarrow.h is included from geoarrow.c
  file(READ ${GEOARROW_C_TEMP} SRC_FILE_CONTENTS)
  file(WRITE ${GEOARROW_C_TEMP} "#include \"geoarrow.h\"\n")
  file(APPEND ${GEOARROW_C_TEMP} "${SRC_FILE_CONTENTS}")

  # Copy geoarrow.hpp
  file(COPY_FILE src/geoarrow/geoarrow.hpp
       ${CMAKE_BINARY_DIR}/amalgamation/geoarrow/geoarrow.hpp)
  file(COPY_FILE src/geoarrow/geoarrow_arrow.hpp
       ${CMAKE_BINARY_DIR}/amalgamation/geoarrow/geoarrow_arrow.hpp)

  # "install" means copy the bundled sources to the specified dir. Just
  # geoarrow.c and geoarrow.h for now.
  install(FILES ${GEOARROW_H_TEMP} ${GEOARROW_C_TEMP} DESTINATION ".")

  # If building tests, add the geoarrow target so we can test against it
  if(GEOARROW_BUILD_TESTS)
    add_library(geoarrow ${GEOARROW_C_TEMP})

    target_include_directories(geoarrow
                               PUBLIC "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/amalgamation/geoarrow>"
                                      "$<INSTALL_INTERFACE:include>")
    target_compile_definitions(geoarrow PUBLIC "$<$<CONFIG:Debug>:GEOARROW_DEBUG>")
    target_link_libraries(geoarrow
                          PRIVATE nanoarrow::nanoarrow
                          PUBLIC coverage_config)
  endif()
else()
  # Normal CMake build
  add_library(geoarrow ${GEOARROW_SOURCES} ${GEOARROW_DOUBLE_PARSE_SOURCE}
                       ${GEOARROW_DOUBLE_PRINT_SOURCE})

  target_include_directories(geoarrow
                             PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/geoarrow>"
                                    "$<INSTALL_INTERFACE:include>")
  target_include_directories(geoarrow
                             PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
  )
  target_compile_definitions(geoarrow PUBLIC "$<$<CONFIG:Debug>:GEOARROW_DEBUG>")
  target_link_libraries(geoarrow
                        PRIVATE nanoarrow::nanoarrow
                        PUBLIC coverage_config)

  install(DIRECTORY src/
          DESTINATION include
          FILES_MATCHING
          PATTERN "*.h")
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/generated/geoarrow_config.h
          DESTINATION include/geoarrow)
endif()

if(TARGET geoarrow)
  install(TARGETS geoarrow DESTINATION lib)

  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
      target_compile_options(geoarrow
                             PRIVATE -Wall
                                     -Werror
                                     -Wextra
                                     -Wpedantic
                                     -Wno-type-limits
                                     -Wmaybe-uninitialized
                                     -Wunused-result
                                     -Wconversion
                                     -Wno-sign-conversion)
    elseif(CMAKE_C_COMPILER_ID STREQUAL "AppleClang" OR CMAKE_C_COMPILER_ID STREQUAL
                                                        "Clang")
      target_compile_options(geoarrow
                             PRIVATE -Wall
                                     -Werror
                                     -Wextra
                                     -Wpedantic
                                     -Wdocumentation
                                     -Wconversion
                                     -Wno-sign-conversion)
    endif()
  endif()
endif()

if(GEOARROW_BUILD_TESTS)
  set(MEMORYCHECK_COMMAND_OPTIONS
      "--leak-check=full --suppressions=${CMAKE_CURRENT_LIST_DIR}/valgrind.supp --error-exitcode=1"
  )
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

  # Give caller the option to link a static version of Arrow C++
  if(GEOARROW_ARROW_STATIC)
    set(GEOARROW_ARROW_TARGET arrow_static)
  else()
    set(GEOARROW_ARROW_TARGET arrow_shared)
  endif()

  # Use an old version of googletest if we have to to support gcc 4.8
  if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_VERSION
                                                 VERSION_GREATER_EQUAL "5.0.0")
    fetchcontent_declare(googletest
                         URL https://github.com/google/googletest/archive/release-1.11.0.zip
                         URL_HASH SHA256=353571c2440176ded91c2de6d6cd88ddd41401d14692ec1f99e35d013feda55a
    )
  else()
    fetchcontent_declare(googletest
                         URL https://github.com/google/googletest/archive/release-1.10.0.zip
                         URL_HASH SHA256=94c634d499558a76fa649edb13721dce6e98fb1e7018dfaeba3cd7a083945e91
    )
  endif()

  fetchcontent_makeavailable(googletest)

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
  add_executable(array_reader_test src/geoarrow/array_reader_test.cc)
  add_executable(array_writer_test src/geoarrow/array_writer_test.cc)
  add_executable(wkx_files_test src/geoarrow/wkx_files_test.cc)
  add_executable(geoarrow_arrow_test src/geoarrow/geoarrow_arrow_test.cc)

  target_link_libraries(geoarrow_hpp_test geoarrow gtest_main)
  target_link_libraries(geoarrow_type_inline_test geoarrow gtest_main
                        nanoarrow::nanoarrow)
  target_link_libraries(builder_test geoarrow gtest_main nanoarrow::nanoarrow)
  target_link_libraries(array_view_test geoarrow gtest_main nanoarrow::nanoarrow)
  target_link_libraries(schema_test geoarrow ${GEOARROW_ARROW_TARGET} gtest_main)
  target_link_libraries(schema_view_test geoarrow gtest_main nanoarrow::nanoarrow)
  target_link_libraries(kernel_test geoarrow gtest_main nanoarrow::nanoarrow)
  target_link_libraries(metadata_test geoarrow gtest_main nanoarrow::nanoarrow)
  target_link_libraries(visitor_test geoarrow gtest_main)
  target_link_libraries(util_test geoarrow gtest_main)
  target_link_libraries(wkb_reader_test geoarrow gtest_main nanoarrow::nanoarrow)
  target_link_libraries(wkb_writer_test geoarrow gtest_main nanoarrow::nanoarrow)
  target_link_libraries(wkt_reader_test geoarrow gtest_main nanoarrow::nanoarrow)
  target_link_libraries(wkt_writer_test geoarrow gtest_main nanoarrow::nanoarrow)
  target_link_libraries(array_reader_test geoarrow gtest_main nanoarrow::nanoarrow)
  target_link_libraries(array_writer_test geoarrow gtest_main nanoarrow::nanoarrow)
  target_link_libraries(wkx_files_test geoarrow gtest_main nanoarrow::nanoarrow)
  target_link_libraries(geoarrow_arrow_test geoarrow ${GEOARROW_ARROW_TARGET} gtest_main)

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
  gtest_discover_tests(array_reader_test)
  gtest_discover_tests(array_writer_test)
  gtest_discover_tests(wkx_files_test)
  gtest_discover_tests(geoarrow_arrow_test)
endif()
