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

# 1. clear var
unset (_DEV_CUDA_HEADER_PATH)
unset (_CUDA_BASE_SOURCE)
unset (_CUDA_OPS_SOURCE)
unset (_DEV_CUDA_DEVICE_SOURCE)
unset (_DEV_CUDA_COMPILER_DEFINES)
unset (_DEV_CUDA_COMPILER_OPTIONS)
unset (_DEV_CUDA_LINKER_OPTIONS)
unset (_DEV_CUDA_LINK_LIBRARIES)


# 2.  set source root path
set(_CUDA_ROOT ${CMAKE_SOURCE_DIR}/source/device/cuda)


# 3.  add header file path
list (APPEND _DEV_CUDA_HEADER_PATH      ${_CUDA_ROOT})
list (APPEND _DEV_CUDA_HEADER_PATH      ${CUDAToolkit_INCLUDE_DIRS})
list (APPEND _DEV_CUDA_HEADER_PATH      ${CUDNN_INCLUDE_DIR})


# 4.  add linking lib searching path
list (APPEND _DEV_CUDA_LINK_PATH        ${CUDAToolkit_LIBRARY_DIR})
list (APPEND _DEV_CUDA_LINK_PATH        ${CUDNN_LIBRARY_DIR})


# 5.  add source files
aux_source_directory("${_CUDA_ROOT}"    _CUDA_BASE_SOURCE)
aux_source_directory("${_CUDA_ROOT}/op" _CUDA_OPS_SOURCE)
list (APPEND _DEV_CUDA_DEVICE_SOURCE    ${_CUDA_BASE_SOURCE})
list (APPEND _DEV_CUDA_DEVICE_SOURCE    ${_CUDA_OPS_SOURCE})


# 6.  add build options for cpu device
# 6.1 is a gcc or clang like compiler
if (TENGINE_COMPILER_GCC OR TENGINE_COMPILER_CLANG)
endif()


# 6.2 is Microsoft Visual C++
if (TENGINE_COMPILER_MSVC)
endif()


# 7.  add link options


# 8.  add link libs
list (APPEND _DEV_CUDA_LINK_LIBRARIES   cudart)
list (APPEND _DEV_CUDA_LINK_LIBRARIES   cublas)
list (APPEND _DEV_CUDA_LINK_LIBRARIES   ${CUDNN_LIBRARY})


# 9. set all to cmake cache
set (TENGINE_CUDA_HEADER_PATH       ${_DEV_CUDA_HEADER_PATH}        CACHE INTERNAL  "Tengine CUDA device header files searching path"   FORCE)
set (TENGINE_CUDA_LINK_PATH         ${_DEV_CUDA_LINK_PATH}          CACHE INTERNAL  "Tengine CUDA device link libraries searching path" FORCE)
set (TENGINE_CUDA_DEVICE_SOURCE     ${_DEV_CUDA_DEVICE_SOURCE}      CACHE INTERNAL  "Tengine CUDA device main source files"             FORCE)
set (TENGINE_CUDA_COMPILER_DEFINES  ${_DEV_CUDA_COMPILER_DEFINES}   CACHE INTERNAL  "Tengine CUDA about compiler defines"               FORCE)
set (TENGINE_CUDA_COMPILER_OPTIONS  ${_DEV_CUDA_COMPILER_OPTIONS}   CACHE INTERNAL  "Tengine CUDA about compiler options"               FORCE)
set (TENGINE_CUDA_LINKER_OPTIONS    ${_DEV_CUDA_LINKER_OPTIONS}     CACHE INTERNAL  "Tengine CUDA about linker options"                 FORCE)
set (TENGINE_CUDA_LINK_LIBRARIES    ${_DEV_CUDA_LINK_LIBRARIES}     CACHE INTERNAL  "Tengine CUDA about link libraries"                 FORCE)
