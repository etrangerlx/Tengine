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
CMAKE_MINIMUM_REQUIRED (VERSION 3.13 FATAL_ERROR)

# Disable inplace builds to prevent source tree corruption.
IF (" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
    MESSAGE (FATAL_ERROR "Tengine Fatal: Building inplace are not allowed. You should create a separate directory for Building.")
ENDIF ()

# set cmake_install_prefix path
IF (NOT DEFINED CMAKE_INSTALL_PREFIX)
    SET (CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Tengine Installation Directory")
ENDIF()

# Enable the languages which in use
ENABLE_LANGUAGE (C CXX)

IF (CMAKE_TOOLCHAIN_FILE)
    SET (LIBRARY_OUTPUT_PATH_ROOT ${CMAKE_BINARY_DIR} CACHE PATH "root for library output, set this to change where android libs are compiled to")

    # get absolute path, but get_filename_component ABSOLUTE only refer with source dir, so find_file here :(
    GET_FILENAME_COMPONENT (CMAKE_TOOLCHAIN_FILE_NAME ${CMAKE_TOOLCHAIN_FILE} NAME)
    FIND_FILE (CMAKE_TOOLCHAIN_FILE ${CMAKE_TOOLCHAIN_FILE_NAME} PATHS ${CMAKE_SOURCE_DIR} NO_DEFAULT_PATH)
    MESSAGE (STATUS "Using CMake tool chain file ${CMAKE_TOOLCHAIN_FILE}")
ENDIF()

IF (NOT CMAKE_BUILD_TYPE)
    SET (CMAKE_BUILD_TYPE release CACHE STRING "Choose the type of build" FORCE)
ENDIF()

# All build options are defined in cmake/options.cmake, grouped by purpose
INCLUDE ("${CMAKE_CURRENT_SOURCE_DIR}/cmake/options.cmake")

# Do check list
INCLUDE ("${CMAKE_CURRENT_SOURCE_DIR}/cmake/check.cmake")
INCLUDE ("${CMAKE_CURRENT_SOURCE_DIR}/cmake/cuda.cmake")
INCLUDE ("${CMAKE_CURRENT_SOURCE_DIR}/cmake/torch.cmake")
INCLUDE ("${CMAKE_CURRENT_SOURCE_DIR}/cmake/utility.cmake")

# Project name
PROJECT (tengine-lite)

# enable assembly language
ENABLE_LANGUAGE (ASM)

# Enable cmake ctest module
ENABLE_TESTING ()

# Set cmake default folder name
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)
SET_PROPERTY(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "cmake")

# Main source files
ADD_SUBDIRECTORY (source)

# Internal modules, not for open source part
INCLUDE (internal/extension.cmake OPTIONAL)

IF (TENGINE_BUILD_BENCHMARK)
    ADD_SUBDIRECTORY(benchmark)
ENDIF()

IF (TENGINE_BUILD_EXAMPLES)
    ADD_SUBDIRECTORY(examples)
ENDIF()

IF (TENGINE_BUILD_DEMO)
    ADD_SUBDIRECTORY(demos)
ENDIF()

IF (TENGINE_BUILD_TESTS)
    ADD_SUBDIRECTORY(tests)
ENDIF()

IF (TENGINE_BUILD_QUANT_TOOL)
    ADD_SUBDIRECTORY(tools/quantize)
ENDIF()

IF (TENGINE_BUILD_CONVERT_TOOL)
    ADD_SUBDIRECTORY(tools/convert_tool)
ENDIF()

# Report summary
INCLUDE ("${CMAKE_CURRENT_SOURCE_DIR}/cmake/summary.cmake")
