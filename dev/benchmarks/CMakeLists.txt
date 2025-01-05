# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

message(STATUS "Building using CMake version: ${CMAKE_VERSION}")
cmake_minimum_required(VERSION 3.18)
include(FetchContent)

project(geoarrow_benchmarks)

if(NOT DEFINED CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
  set(CMAKE_C_STANDARD_REQUIRED ON)
endif()

if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

set(GEOARROW_BENCHMARK_VERSION
    ""
    CACHE STRING "geoarrow version to benchmark")
set(GEOARROW_BENCHMARK_SOURCE_DIR
    ""
    CACHE STRING "path to a geoarrow source checkout to benchmark" OFF)

# Avoids a warning about timestamps on downloaded files (prefer new policy
# if available))
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.23")
  cmake_policy(SET CMP0135 NEW)
endif()

# Use google/benchmark
set(BENCHMARK_ENABLE_TESTING OFF)
fetchcontent_declare(benchmark
                     URL https://github.com/google/benchmark/archive/refs/tags/v1.8.3.zip
                     URL_HASH SHA256=abfc22e33e3594d0edf8eaddaf4d84a2ffc491ad74b6a7edc6e7a608f690e691
)
fetchcontent_makeavailable(benchmark)

if(IS_DIRECTORY "${GEOARROW_BENCHMARK_SOURCE_URL}")
  fetchcontent_declare(geoarrow SOURCE_DIR "${GEOARROW_BENCHMARK_SOURCE_URL}")
  fetchcontent_makeavailable(geoarrow)
elseif(NOT "${GEOARROW_BENCHMARK_SOURCE_URL}" STREQUAL "")
  fetchcontent_declare(geoarrow URL "${GEOARROW_BENCHMARK_SOURCE_URL}")
  fetchcontent_makeavailable(geoarrow)
endif()

# Check that either the parent scope or this CMakeLists.txt defines a geoarrow target
if(NOT TARGET geoarrow)
  message(FATAL_ERROR "geoarrow target not found (missing -DGEOARROW_BENCHMARK_SOURCE_URL option?)"
  )
endif()

# Add executables and register them as tests.
# This lets all benchmarks run via ctest -VV when this is the top-level project
# and takes care of setting the relevant test properties such that the benchmarks
# can find the fixtures.
include(CTest)
enable_testing()

foreach(ITEM array_view)
  add_executable(${ITEM}_benchmark "c/${ITEM}_benchmark.cc")
  target_link_libraries(${ITEM}_benchmark PRIVATE geoarrow benchmark::benchmark_main)
  add_test(NAME ${ITEM}_benchmark COMMAND ${ITEM}_benchmark
                                          --benchmark_out=${ITEM}_benchmark.json)
  set_tests_properties(${ITEM}_benchmark PROPERTIES WORKING_DIRECTORY
                                                    "${CMAKE_BINARY_DIR}")
endforeach(ITEM)
