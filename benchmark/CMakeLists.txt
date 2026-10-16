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

# generate tengine header file
file (MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tengine)
file (COPY ${CMAKE_SOURCE_DIR}/source/api/c_api.h DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/tengine)


# examples function
function (TENGINE_BENCHMARK name)
    add_executable (${name})

    # add common source file
    target_sources (${name} PRIVATE common/timer.cc)

    # add source files
    foreach (_VAR ${ARGN})
        target_sources (${name} PRIVATE ${_VAR})
    endforeach()

    # header file path
    target_include_directories (${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories (${name} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

    # add linking library
    target_link_libraries (${name} PRIVATE ${CMAKE_PROJECT_NAME})

    # add to a virtual project group
    set_property(TARGET ${name} PROPERTY FOLDER "benchmark")
endfunction()


# add benchmark
tengine_benchmark (tm_benchmark      tm_benchmark.cc)
