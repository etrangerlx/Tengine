# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# License); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# AS IS BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
# Copyright (c) 2021, OPEN AI LAB
# Author: lswang@openailab.com
#

# Check CMake version
cmake_minimum_required (VERSION 3.13 FATAL_ERROR)

# Disable inplace builds to prevent source tree corruption.
if (" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
    message (FATAL_ERROR "Tengine Fatal: Building inplace are not allowed. You should create a separate directory for Building.")
endif ()

# set cmake_install_prefix path
if (NOT DEFINED CMAKE_INSTALL_PREFIX)
    set (CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Tengine Installation Directory")
endif()

# Enable the languages which in use
enable_language (C CXX)

if (CMAKE_TOOLCHAIN_FILE)
    set (LIBRARY_OUTPUT_PATH_ROOT ${CMAKE_BINARY_DIR} CACHE PATH "root for library output, set this to change where android libs are compiled to")

    # get absolute path, but get_filename_component ABSOLUTE only refer with source dir, so find_file here :(
    get_filename_component (CMAKE_TOOLCHAIN_FILE_NAME ${CMAKE_TOOLCHAIN_FILE} NAME)
    find_file (CMAKE_TOOLCHAIN_FILE ${CMAKE_TOOLCHAIN_FILE_NAME} PATHS ${CMAKE_SOURCE_DIR} NO_DEFAULT_PATH)
    message (STATUS "Using CMake tool chain file ${CMAKE_TOOLCHAIN_FILE}")
endif()

if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE release CACHE STRING "Choose the type of build" FORCE)
endif()

# All build options are defined in cmake/options.cmake, grouped by purpose
include ("${CMAKE_CURRENT_SOURCE_DIR}/cmake/options.cmake")

# Do check list
include ("${CMAKE_CURRENT_SOURCE_DIR}/cmake/check.cmake")
include ("${CMAKE_CURRENT_SOURCE_DIR}/cmake/cuda.cmake")
include ("${CMAKE_CURRENT_SOURCE_DIR}/cmake/torch.cmake")
include ("${CMAKE_CURRENT_SOURCE_DIR}/cmake/utility.cmake")

# Project name
project (tengine-lite)

# enable assembly language
enable_language (ASM)

# Enable cmake ctest module
enable_testing ()

# Set cmake default folder name
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "cmake")

# Main source files
add_subdirectory (source)

# Internal modules, not for open source part
include (internal/extension.cmake OPTIONAL)

if (TENGINE_BUILD_BENCHMARK)
    add_subdirectory(benchmark)
endif()

if (TENGINE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if (TENGINE_BUILD_DEMO)
    add_subdirectory(demos)
endif()

if (TENGINE_BUILD_TESTS)
    add_subdirectory(tests)
endif()

if (TENGINE_BUILD_QUANT_TOOL)
    add_subdirectory(tools/quantize)
endif()

if (TENGINE_BUILD_CONVERT_TOOL)
    add_subdirectory(tools/convert_tool)
endif()

# Report summary
include ("${CMAKE_CURRENT_SOURCE_DIR}/cmake/summary.cmake")
