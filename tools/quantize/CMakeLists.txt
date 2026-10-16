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
# Author: qtang@openailab.com
#

unset (TENGINE_TOOL_OPTIONS)
unset (TENGINE_TOOL_LINK_LIBRARIES)

find_package(OpenCV REQUIRED COMPONENTS core imgcodecs imgproc)

if (${TENGINE_TARGET_PROCESSOR} MATCHES "X86")
    # macro for adding quant tool
    function (TENGINE_QUANT_TOOL name file)
        add_executable(
            ${name}
            ./quant_save_graph.cpp
            ./algorithm/quant_dfq.cpp
            ./algorithm/quant_eq.cpp
            ./quant_utils.cpp
            ../save_graph/save_graph.cpp
            ../save_graph/tm2_op_save.cpp
            ../save_graph/tm2_generate.c
            "${CMAKE_CURRENT_SOURCE_DIR}/${file}")
        target_include_directories (${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        target_include_directories (${name} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})       
        target_include_directories (${name} PRIVATE "${PROJECT_BINARY_DIR}")     
        target_include_directories (${name} PRIVATE "${PROJECT_BINARY_DIR}/source")
        target_include_directories (${name} PRIVATE "${PROJECT_SOURCE_DIR}/source")
        target_include_directories (${name} PRIVATE "${PROJECT_SOURCE_DIR}/source/operator/prototype")       
        target_include_directories (${name} PRIVATE "${PROJECT_SOURCE_DIR}/tools/quantize/common")
        target_include_directories (${name} PRIVATE "${PROJECT_SOURCE_DIR}/tools/save_graph")

        if (${TENGINE_COVERAGE})
            list (APPEND TENGINE_TOOL_OPTIONS "-fprofile-arcs")
            list (APPEND TENGINE_TOOL_OPTIONS "-ftest-coverage")
            list (APPEND TENGINE_TOOL_LINK_LIBRARIES -coverage)
            list (APPEND TENGINE_TOOL_LINK_LIBRARIES -lgcov)
        endif()
        target_compile_options (${name} PRIVATE ${TENGINE_TOOL_OPTIONS})
        target_link_libraries (${name} PRIVATE ${CMAKE_PROJECT_NAME}-static ${OpenCV_LIBS} ${TENGINE_TOOL_LINK_LIBRARIES})

        install (TARGETS ${name} DESTINATION bin)

        # add to a virtual project group
        set_property(TARGET ${name} PROPERTY FOLDER "tools/quant_tool")
    endfunction()

    tengine_quant_tool(quant_tool_int8      quant_tool_int8.cpp)
    tengine_quant_tool(quant_tool_uint8     quant_tool_uint8.cpp)
    tengine_quant_tool(quant_tool_uint8_perchannel     quant_tool_uint8_perchannel.cpp)
else()
    message (FATAL_ERROR "quantization tool can only be built on x86")
endif()
