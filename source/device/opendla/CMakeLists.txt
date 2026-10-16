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
# Copyright (c) 2021, Institute of Computing Technology
# Author: wanglei21c@mails.ucas.ac.cn
#

# 0. clear var
unset (_DEV_ODLA_HEADER_PATH)
unset (_ODLA_BASE_SOURCE)
unset (_ODLA_OPS_SOURCE)
unset (_DEV_ODLA_DEVICE_SOURCE)
unset (_DEV_ODLA_COMPILER_DEFINES)
unset (_DEV_ODLA_COMPILER_OPTIONS)
unset (_DEV_ODLA_LINKER_OPTIONS)
unset (_DEV_ODLA_LINK_LIBRARIES)


# 1.  set source root path
set(_ODLA_ROOT ${CMAKE_SOURCE_DIR}/source/device/opendla)

# 2.  add header file path
list (APPEND _DEV_ODLA_HEADER_PATH        ${_ODLA_ROOT})
list (APPEND _DEV_ODLA_HEADER_PATH        ${CMAKE_CURRENT_BINARY_DIR})
list (APPEND _DEV_ODLA_HEADER_PATH        ${CMAKE_SOURCE_DIR}/source/device/opendla/include/)
list (APPEND _DEV_ODLA_HEADER_PATH        ${CMAKE_SOURCE_DIR}/source/device/opendla/include/common)
list (APPEND _DEV_ODLA_HEADER_PATH        ${CMAKE_SOURCE_DIR}/source/device/opendla/include/compiler)
list (APPEND _DEV_ODLA_HEADER_PATH        ${CMAKE_SOURCE_DIR}/source/device/opendla/include/runtime)
list (APPEND _DEV_ODLA_HEADER_PATH        ${CMAKE_SOURCE_DIR}/source/device/opendla/include/external)


# 3.
list (APPEND _DEV_ODLA_COMPILER_DEFINES    NVDLA_UTILS_ERROR_TAG="DLA")

# 3.  add source files
aux_source_directory ("${_ODLA_ROOT}"     _ODLA_BASE_SOURCE)
aux_source_directory ("${_ODLA_ROOT}/op"  _ODLA_OPS_SOURCE)

list (APPEND _DEV_ODLA_DEVICE_SOURCE      ${_ODLA_BASE_SOURCE})
list (APPEND _DEV_ODLA_DEVICE_SOURCE      ${_ODLA_OPS_SOURCE})

set (_ODLA_LIB_NAME "odla")
add_library (${_ODLA_LIB_NAME} STATIC)
target_sources (${_ODLA_LIB_NAME} PRIVATE )
target_sources (${_ODLA_LIB_NAME} PRIVATE ${_ODLA_BASE_SOURCE})
target_sources (${_ODLA_LIB_NAME} PRIVATE ${_ODLA_OPS_SOURCE})
# 4.  add build options for cpu device
# 4.1 is a gcc or clang like compiler
if (TENGINE_COMPILER_GCC OR TENGINE_COMPILER_CLANG)
endif()


# 4.2 is Microsoft Visual C++
if (TENGINE_COMPILER_MSVC)
endif()


# 5.  add link options

# 6.  add link libs
list (APPEND _DEV_ODLA_LINK_PATH            ${_ODLA_ROOT}/lib)
list (APPEND _DEV_ODLA_LINK_LIBRARIES       "nvdla_compiler" )
list (APPEND _DEV_ODLA_LINK_LIBRARIES       "libprotobuf.a" )
list (APPEND _DEV_ODLA_LINK_LIBRARIES       "pthread")
list (APPEND _DEV_ODLA_LINK_LIBRARIES       "nvdla_runtime" )


# 7. set all to cmake cache
set (TENGINE_ODLA_HEADER_PATH       ${_DEV_ODLA_HEADER_PATH}        CACHE INTERNAL  "Tengine ODLA device header files searching path"   FORCE)
set (TENGINE_ODLA_LINK_PATH         ${_DEV_ODLA_LINK_PATH}          CACHE INTERNAL  "Tengine ODLA device link libraries searching path" FORCE)
set (TENGINE_ODLA_DEVICE_SOURCE     ${_DEV_ODLA_DEVICE_SOURCE}      CACHE INTERNAL  "Tengine ODLA device main source files"             FORCE)
set (TENGINE_ODLA_COMPILER_DEFINES  ${_DEV_ODLA_COMPILER_DEFINES}   CACHE INTERNAL  "Tengine ODLA about compiler defines"               FORCE)
set (TENGINE_ODLA_COMPILER_OPTIONS  ${_DEV_ODLA_COMPILER_OPTIONS}   CACHE INTERNAL  "Tengine ODLA about compiler options"               FORCE)
set (TENGINE_ODLA_LINKER_OPTIONS    ${_DEV_ODLA_LINKER_OPTIONS}     CACHE INTERNAL  "Tengine ODLA about linker options"                 FORCE)
set (TENGINE_ODLA_LINK_LIBRARIES    ${_DEV_ODLA_LINK_LIBRARIES}     CACHE INTERNAL  "Tengine ODLA about link libraries"                 FORCE)


# 8. install device option
install (FILES ${CMAKE_CURRENT_BINARY_DIR}/odla_define.h DESTINATION include/tengine RENAME odla_device.h)
