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
cmake_minimum_required(VERSION 3.12)

project(GeoArrowExampleCMakeMinimal LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)

option(FIND_GEOARROW "Find an existing geoarrow" ON)

# When adding geoarrow's CMake directory to a CMake project that contains a library
# intended for use by others, set GEOARROW_NAMESPACE to rename symbols in the
# geoarrow library such that they do not collide with other libraries that may also
# link to their own copy of geoarrow. You may wish to include the namespace only
# on release builds, since the namespace implementation obscures inline help
# available in many text editors.
set(GEOARROW_NAMESPACE "ExampleCmakeScenarios")

include(FetchContent)

if(FIND_GEOARROW)
  # Users should pin to a specific version of geoarrow if they choose
  # to find geoarrow rather than pinning to a specific version via
  # FetchContent.
  message(STATUS "Finding installed geoarrow")
  find_package(geoarrow REQUIRED)
  message(STATUS "Found geoarrow ${geoarrow_VERSION}")
else()
  # Using SOURCE_DIR here such that CI accurately reflects the state of the
  # source; however, using GIT_TAG or URL of a pinned version is a more realistic
  # user pattern.
  fetchcontent_declare(geoarrow SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
  fetchcontent_makeavailable(geoarrow)
endif()

message(IMPORTED_TARGETS=${IMPORTED_TARGETS})
message(BUILDSYSTEM_TARGETS=${BUILDSYSTEM_TARGETS})

add_executable(minimal_cpp_app src/app.cc)
target_link_libraries(minimal_cpp_app PRIVATE geoarrow::geoarrow)
