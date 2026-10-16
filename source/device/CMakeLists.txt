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
#
# Device module: aggregates all hardware backends.
# Each enabled backend exports TENGINE_<NAME>_* variables (legacy plugin
# interface) which are merged here. The cpu backend additionally builds its
# own object target tengine_device_cpu.

# unset for regeneration
unset (_TENGINE_DEVICE_SOURCE)
unset (_TENGINE_DEVICE_HEADER_PATH)
unset (_TENGINE_DEVICE_LINK_PATH)
unset (_TENGINE_DEVICE_COMPILER_DEFINES)
unset (_TENGINE_DEVICE_COMPILER_OPTIONS)
unset (_TENGINE_DEVICE_LINKER_OPTIONS)
unset (_TENGINE_DEVICE_LINK_LIBRARIES)


# include each device backend, manually for now
add_subdirectory (cpu)


# device level source files: the device registration glue code (device.c)
# NOTE: backend sources stay in the variables below and are compiled into
# the final aggregate targets, until each backend is migrated to its own target
file (GLOB _DEVICE_SOURCE CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/source/device/*.c)

# add to src group
tengine_source_group ("device" ${_DEVICE_SOURCE})


# for CPU: merge cpu build attributes, the cpu sources themselves live in
# the tengine_device_cpu object library and must NOT be appended here again
list (APPEND _TENGINE_DEVICE_HEADER_PATH        ${TENGINE_CPU_HEADER_PATH})
list (APPEND _TENGINE_DEVICE_COMPILER_DEFINES   ${TENGINE_CPU_COMPILER_DEFINES})
list (APPEND _TENGINE_DEVICE_COMPILER_OPTIONS   ${TENGINE_CPU_COMPILER_OPTIONS})
list (APPEND _TENGINE_DEVICE_LINKER_OPTIONS     ${TENGINE_CPU_LINKER_OPTIONS})
list (APPEND _TENGINE_DEVICE_LINK_LIBRARIES     ${TENGINE_CPU_LINK_LIBRARIES})


# Arm Compute Library
if (TENGINE_ENABLE_ACL)
    add_subdirectory (acl)

    list (APPEND _TENGINE_DEVICE_HEADER_PATH        ${TENGINE_ACL_HEADER_PATH})
    list (APPEND _TENGINE_DEVICE_LINK_PATH          ${TENGINE_ACL_LINK_PATH})
    list (APPEND _TENGINE_DEVICE_COMPILER_DEFINES   ${TENGINE_ACL_COMPILER_DEFINES})
    list (APPEND _TENGINE_DEVICE_COMPILER_OPTIONS   ${TENGINE_ACL_COMPILER_OPTIONS})
    list (APPEND _TENGINE_DEVICE_LINKER_OPTIONS     ${TENGINE_ACL_LINKER_OPTIONS})
    list (APPEND _TENGINE_DEVICE_LINK_LIBRARIES     ${TENGINE_ACL_LINK_LIBRARIES})
    list (APPEND _TENGINE_DEVICE_SOURCE             ${TENGINE_ACL_DEVICE_SOURCE})
    list (APPEND _TENGINE_DEVICE_COMPILER_DEFINES   TENGINE_ENABLE_ACL)
endif()


# nVIDIA CUDA
if (TENGINE_ENABLE_CUDA)
    add_subdirectory (cuda)

    list (APPEND _TENGINE_DEVICE_HEADER_PATH        ${TENGINE_CUDA_HEADER_PATH})
    list (APPEND _TENGINE_DEVICE_LINK_PATH          ${TENGINE_CUDA_LINK_PATH})
    list (APPEND _TENGINE_DEVICE_COMPILER_DEFINES   ${TENGINE_CUDA_COMPILER_DEFINES})
    list (APPEND _TENGINE_DEVICE_COMPILER_OPTIONS   ${TENGINE_CUDA_COMPILER_OPTIONS})
    list (APPEND _TENGINE_DEVICE_LINKER_OPTIONS     ${TENGINE_CUDA_LINKER_OPTIONS})
    list (APPEND _TENGINE_DEVICE_LINK_LIBRARIES     ${TENGINE_CUDA_LINK_LIBRARIES})
    list (APPEND _TENGINE_DEVICE_SOURCE             ${TENGINE_CUDA_DEVICE_SOURCE})
    list (APPEND _TENGINE_DEVICE_COMPILER_DEFINES   TENGINE_ENABLE_CUDA)
endif()


# Khronos OpenCL
if (TENGINE_ENABLE_OPENCL)
    add_subdirectory (opencl)

    list (APPEND _TENGINE_DEVICE_HEADER_PATH        ${TENGINE_OCL_HEADER_PATH})
    list (APPEND _TENGINE_DEVICE_LINK_PATH          ${TENGINE_OCL_LINK_PATH})
    list (APPEND _TENGINE_DEVICE_COMPILER_DEFINES   ${TENGINE_OCL_COMPILER_DEFINES})
    list (APPEND _TENGINE_DEVICE_COMPILER_OPTIONS   ${TENGINE_OCL_COMPILER_OPTIONS})
    list (APPEND _TENGINE_DEVICE_LINKER_OPTIONS     ${TENGINE_OCL_LINKER_OPTIONS})
    list (APPEND _TENGINE_DEVICE_LINK_LIBRARIES     ${TENGINE_OCL_LINK_LIBRARIES})
    list (APPEND _TENGINE_DEVICE_SOURCE             ${TENGINE_OCL_DEVICE_SOURCE})
    list (APPEND _TENGINE_DEVICE_COMPILER_DEFINES   TENGINE_ENABLE_OPENCL)
endif()


# OPENDLA
if (TENGINE_ENABLE_OPENDLA)
    add_subdirectory (opendla)

    list (APPEND _TENGINE_DEVICE_HEADER_PATH        ${TENGINE_ODLA_HEADER_PATH})
    list (APPEND _TENGINE_DEVICE_LINK_PATH          ${TENGINE_ODLA_LINK_PATH})
    list (APPEND _TENGINE_DEVICE_COMPILER_DEFINES   ${TENGINE_ODLA_COMPILER_DEFINES})
    list (APPEND _TENGINE_DEVICE_COMPILER_OPTIONS   ${TENGINE_ODLA_COMPILER_OPTIONS})
    list (APPEND _TENGINE_DEVICE_LINKER_OPTIONS     ${TENGINE_ODLA_LINKER_OPTIONS})
    list (APPEND _TENGINE_DEVICE_LINK_LIBRARIES     ${TENGINE_ODLA_LINK_LIBRARIES})
    list (APPEND _TENGINE_DEVICE_SOURCE             ${TENGINE_ODLA_DEVICE_SOURCE})
    list (APPEND _TENGINE_DEVICE_COMPILER_DEFINES   TENGINE_ENABLE_OPENDLA)
endif()


# nVIDIA TensorRT
if (TENGINE_ENABLE_TENSORRT)
    add_subdirectory (tensorrt)

    list (APPEND _TENGINE_DEVICE_HEADER_PATH        ${TENGINE_TENSORRT_HEADER_PATH})
    list (APPEND _TENGINE_DEVICE_LINK_PATH          ${TENGINE_TENSORRT_LINK_PATH})
    list (APPEND _TENGINE_DEVICE_COMPILER_DEFINES   ${TENGINE_TENSORRT_COMPILER_DEFINES})
    list (APPEND _TENGINE_DEVICE_COMPILER_OPTIONS   ${TENGINE_TENSORRT_COMPILER_OPTIONS})
    list (APPEND _TENGINE_DEVICE_LINKER_OPTIONS     ${TENGINE_TENSORRT_LINKER_OPTIONS})
    list (APPEND _TENGINE_DEVICE_LINK_LIBRARIES     ${TENGINE_TENSORRT_LINK_LIBRARIES})
    list (APPEND _TENGINE_DEVICE_SOURCE             ${TENGINE_TENSORRT_DEVICE_SOURCE})
    list (APPEND _TENGINE_DEVICE_COMPILER_DEFINES   TENGINE_ENABLE_TENSORRT)
endif()


# VeriSilicon TIM-VX
if (TENGINE_ENABLE_TIM_VX)
    add_subdirectory (tim-vx)

    list (APPEND _TENGINE_DEVICE_HEADER_PATH        ${TENGINE_TIM_VX_HEADER_PATH})
    list (APPEND _TENGINE_DEVICE_LINK_PATH          ${TENGINE_TIM_VX_LINK_PATH})
    list (APPEND _TENGINE_DEVICE_COMPILER_DEFINES   ${TENGINE_TIM_VX_COMPILER_DEFINES})
    list (APPEND _TENGINE_DEVICE_COMPILER_OPTIONS   ${TENGINE_TIM_VX_COMPILER_OPTIONS})
    list (APPEND _TENGINE_DEVICE_LINKER_OPTIONS     ${TENGINE_TIM_VX_LINKER_OPTIONS})
    list (APPEND _TENGINE_DEVICE_LINK_LIBRARIES     ${TENGINE_TIM_VX_LINK_LIBRARIES})
    list (APPEND _TENGINE_DEVICE_SOURCE             ${TENGINE_TIM_VX_DEVICE_SOURCE})
    list (APPEND _TENGINE_DEVICE_COMPILER_DEFINES   TENGINE_ENABLE_TIM_VX)
endif()


# libTorch
if (TENGINE_ENABLE_TORCH)
    add_subdirectory (torch)

    list (APPEND _TENGINE_DEVICE_HEADER_PATH        ${TENGINE_TORCH_HEADER_PATH})
    list (APPEND _TENGINE_DEVICE_LINK_PATH          ${TENGINE_TORCH_LINK_PATH})
    list (APPEND _TENGINE_DEVICE_COMPILER_DEFINES   ${TENGINE_TORCH_COMPILER_DEFINES})
    list (APPEND _TENGINE_DEVICE_COMPILER_OPTIONS   ${TENGINE_TORCH_COMPILER_OPTIONS})
    list (APPEND _TENGINE_DEVICE_LINKER_OPTIONS     ${TENGINE_TORCH_LINKER_OPTIONS})
    list (APPEND _TENGINE_DEVICE_LINK_LIBRARIES     ${TENGINE_TORCH_LINK_LIBRARIES})
    list (APPEND _TENGINE_DEVICE_SOURCE             ${TENGINE_TORCH_DEVICE_SOURCE})
    list (APPEND _TENGINE_DEVICE_COMPILER_DEFINES   TENGINE_ENABLE_TORCH)
endif()


# Khronos Vulkan
if (TENGINE_ENABLE_VULKAN)
    add_subdirectory (vulkan)

    list (APPEND _TENGINE_DEVICE_HEADER_PATH        ${TENGINE_VULKAN_HEADER_PATH})
    list (APPEND _TENGINE_DEVICE_LINK_PATH          ${TENGINE_VULKAN_LINK_PATH})
    list (APPEND _TENGINE_DEVICE_COMPILER_DEFINES   ${TENGINE_VULKAN_COMPILER_DEFINES})
    list (APPEND _TENGINE_DEVICE_COMPILER_OPTIONS   ${TENGINE_VULKAN_COMPILER_OPTIONS})
    list (APPEND _TENGINE_DEVICE_LINKER_OPTIONS     ${TENGINE_VULKAN_LINKER_OPTIONS})
    list (APPEND _TENGINE_DEVICE_LINK_LIBRARIES     ${TENGINE_VULKAN_LINK_LIBRARIES})
    list (APPEND _TENGINE_DEVICE_SOURCE             ${TENGINE_VULKAN_DEVICE_SOURCE})
    list (APPEND _TENGINE_DEVICE_DEPENDS_FORWARD    ${TENGINE_VULKAN_DEPENDS_FORWARD})
    list (APPEND _TENGINE_DEVICE_COMPILER_DEFINES   TENGINE_ENABLE_VULKAN)
endif()


# module target: device registration glue code, one per enabled backend switch
add_library (tengine_core_device OBJECT ${_DEVICE_SOURCE})
target_link_libraries (tengine_core_device PRIVATE tengine_build_config tengine_public_include)
target_include_directories (tengine_core_device PRIVATE ${_TENGINE_DEVICE_HEADER_PATH})
target_compile_definitions (tengine_core_device PRIVATE ${_TENGINE_DEVICE_COMPILER_DEFINES})


# set var to cache
set (TENGINE_DEVICE_HEADER_PATH         ${_TENGINE_DEVICE_HEADER_PATH}      CACHE INTERNAL "Tengine device level header files searching path" FORCE)
set (TENGINE_DEVICE_LINK_PATH           ${_TENGINE_DEVICE_LINK_PATH}        CACHE INTERNAL "Tengine device level link libraries searching path" FORCE)
set (TENGINE_DEVICE_SOURCE              ${_TENGINE_DEVICE_SOURCE}           CACHE INTERNAL "Tengine device level source files, backend plugin sources only" FORCE)
set (TENGINE_DEVICE_COMPILER_DEFINES    ${_TENGINE_DEVICE_COMPILER_DEFINES} CACHE INTERNAL "Tengine device about compiler defines" FORCE)
set (TENGINE_DEVICE_COMPILER_OPTIONS    ${_TENGINE_DEVICE_COMPILER_OPTIONS} CACHE INTERNAL "Tengine device about compiler options" FORCE)
set (TENGINE_DEVICE_LINKER_OPTIONS      ${_TENGINE_DEVICE_LINKER_OPTIONS}   CACHE INTERNAL "Tengine device about linker options" FORCE)
set (TENGINE_DEVICE_LINK_LIBRARIES      ${_TENGINE_DEVICE_LINK_LIBRARIES}   CACHE INTERNAL "Tengine device about link libraries" FORCE)
set (TENGINE_DEVICE_DEPENDS_FORWARD     ${TENGINE_DEVICE_DEPENDS_FORWARD}   CACHE INTERNAL "Tengine device about depends project" FORCE)
