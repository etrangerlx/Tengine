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

# unset for generation
unset (_DEV_CPU_HEADER_PATH)
unset (_CPU_OP_DIR)
unset (_CPU_OP_LIST)
unset (_CPU_DEVICE_SOURCE)
unset (_CPU_REF_SOURCE)
unset (_CPU_HCL_SOURCE)
unset (_CPU_CORTEX_A_SOURCE)
unset (_CPU_CORTEX_A_V7_SOURCE)
unset (_CPU_CORTEX_A_V7_KERNEL)
unset (_CPU_CORTEX_A_V8_SOURCE)
unset (_CPU_CORTEX_A_V8_KERNEL)
unset (_CPU_CORTEX_A_V8_2_SOURCE)
unset (_CPU_CORTEX_A_V8_2_KERNEL)
unset (_CPU_CORTEX_M_SOURCE)
unset (_CPU_CORTEX_M_KERNEL)
unset (_CPU_x86_SOURCE)
unset (_CPU_x86_KERNEL)
unset (_CPU_MIPS_SOURCE)
unset (_CPU_MIPS_KERNEL)
unset (_CPU_REGISTER_SOURCE)

unset (_CPU_COMPILER_DEFINES)
unset (_CPU_COMPILER_OPTIONS)



# 0. add header file path
list (APPEND _DEV_CPU_HEADER_PATH "${CMAKE_SOURCE_DIR}/source/device")
list (APPEND _DEV_CPU_HEADER_PATH "${CMAKE_BINARY_DIR}/source/device/cpu")
list (APPEND _DEV_CPU_HEADER_PATH "${CMAKE_SOURCE_DIR}/source/device/cpu")
list (APPEND _DEV_CPU_HEADER_PATH "${CMAKE_SOURCE_DIR}/source/operator/prototype")


# 1. add source files
# 1.1 set source root path
set(_OP_ROOT ${CMAKE_SOURCE_DIR}/source/device/cpu/op)


# 1.2 get all op folder from source dir
file (GLOB _CPU_OP_DIR RELATIVE ${_OP_ROOT} ${_OP_ROOT}/*)


# 1.3 check if define allowed operators list
if ((DEFINED TENGINE_CPU_OPS_LIMIT_FILE) AND (EXISTS ${TENGINE_CPU_OPS_LIMIT_FILE}))
    # 1.3.1 get allowed op options
    include ("${TENGINE_CPU_OPS_LIMIT_FILE}")

    # 1.3.2 check if op is enabled
    foreach(_DIR ${_CPU_OP_DIR})
        if (FIND _CPU_OP_LIST ${_OP_NAME} _OP_NAME_FOUND)
            if ((NOT _OP_NAME_FOUND) AND ${TENGINE_OP_${_OP_NAME}})
                list (APPEND _CPU_OP_LIST ${_OP_NAME})
                if(${TENGINE_VERBOSE})
                    message(AUTHOR_WARNING "  add op ${_OP_NAME}")
                endif()
            endif()
        endif()
    endforeach()
else()
    # 1.3.1 if not set limit file, all op is allowed
    foreach(_DIR ${_CPU_OP_DIR})
        list (APPEND _CPU_OP_LIST ${_DIR})
        if(${TENGINE_VERBOSE})
            message(AUTHOR_WARNING "  add op ${_DIR}")
        endif()
    endforeach()
endif()


# 1.4 collect all source files
# 1.4.1 collect source files for cpu device
file (GLOB _CPU_DEVICE_SOURCE "${CMAKE_SOURCE_DIR}/source/device/cpu/*.c")

# 1.4.2 collect source files for cpu operators
foreach(_OP_NAME ${_CPU_OP_LIST})
    # base op files should be placed at ${_OP_NAME}/
    # base op main file should be name with "${_OP_NAME}_ref.c"
    file (GLOB _REF_SOURCE                  "${_OP_ROOT}/${_OP_NAME}/*.c")

    file (GLOB _CORTEX_A_SOURCE             "${_OP_ROOT}/${_OP_NAME}/cortex-a/*.c")
    file (GLOB _CORTEX_A_V7_SOURCE          "${_OP_ROOT}/${_OP_NAME}/cortex-a/armv7/*.c")
    file (GLOB _CORTEX_A_V7_KERNEL          "${_OP_ROOT}/${_OP_NAME}/cortex-a/armv7/*.S")
    file (GLOB _CORTEX_A_V8_SOURCE          "${_OP_ROOT}/${_OP_NAME}/cortex-a/armv8/*.c")
    file (GLOB _CORTEX_A_V8_KERNEL          "${_OP_ROOT}/${_OP_NAME}/cortex-a/armv8/*.S")
    file (GLOB _CORTEX_A_V8_2_SOURCE        "${_OP_ROOT}/${_OP_NAME}/cortex-a/armv8.2/*.c")
    file (GLOB _CORTEX_A_V8_2_KERNEL        "${_OP_ROOT}/${_OP_NAME}/cortex-a/armv8.2/*.S")

    file (GLOB _CORTEX_M_SOURCE             "${_OP_ROOT}/${_OP_NAME}/cortex-m/*.c")
    file (GLOB _CORTEX_M_KERNEL             "${_OP_ROOT}/${_OP_NAME}/cortex-m/*.S")

    file (GLOB _x86_SOURCE                  "${_OP_ROOT}/${_OP_NAME}/x86/*.c")
    file (GLOB _x86_KERNEL                  "${_OP_ROOT}/${_OP_NAME}/x86/*.S")

    file (GLOB _MIPS_SOURCE                 "${_OP_ROOT}/${_OP_NAME}/mips/*.c")
    file (GLOB _MIPS_KERNEL                 "${_OP_ROOT}/${_OP_NAME}/mips/*.S")

    file (GLOB _RISC_V_SOURCE               "${_OP_ROOT}/${_OP_NAME}/risc-v/*.c")
    file (GLOB _RISC_V_KERNEL               "${_OP_ROOT}/${_OP_NAME}/risc-v/*.S")
    file (GLOB _RISC_V_LP64DV_SOURCE        "${_OP_ROOT}/${_OP_NAME}/risc-v/lp64dv/*.c")
    file (GLOB _RISC_V_LP64DV_KERNEL        "${_OP_ROOT}/${_OP_NAME}/risc-v/lp64dv/*.S")

    list (APPEND _CPU_REF_SOURCE            ${_REF_SOURCE})
    list (APPEND _CPU_CORTEX_A_SOURCE       ${_CORTEX_A_SOURCE})
    list (APPEND _CPU_CORTEX_A_V7_SOURCE    ${_CORTEX_A_V7_SOURCE})
    list (APPEND _CPU_CORTEX_A_V7_KERNEL    ${_CORTEX_A_V7_KERNEL})
    list (APPEND _CPU_CORTEX_A_V8_SOURCE    ${_CORTEX_A_V8_SOURCE})
    list (APPEND _CPU_CORTEX_A_V8_KERNEL    ${_CORTEX_A_V8_KERNEL})
    list (APPEND _CPU_CORTEX_A_V8_2_SOURCE  ${_CORTEX_A_V8_2_SOURCE})
    list (APPEND _CPU_CORTEX_A_V8_2_KERNEL  ${_CORTEX_A_V8_2_KERNEL})
    list (APPEND _CPU_CORTEX_M_SOURCE       ${_CORTEX_M_SOURCE})
    list (APPEND _CPU_CORTEX_M_KERNEL       ${_CORTEX_M_KERNEL})
    list (APPEND _CPU_x86_SOURCE            ${_x86_SOURCE})
    list (APPEND _CPU_x86_KERNEL            ${_x86_KERNEL})
    list (APPEND _CPU_MIPS_SOURCE           ${_MIPS_SOURCE})
    list (APPEND _CPU_MIPS_KERNEL           ${_MIPS_KERNEL})
    list (APPEND _CPU_RISC_V_LP64DV_SOURCE  ${_RISC_V_LP64DV_SOURCE})
    list (APPEND _CPU_RISC_V_LP64DV_KERNEL  ${_RISC_V_LP64DV_KERNEL})
endforeach()


# 1.5 collect all need be registered files
foreach(_OP_NAME ${_CPU_OP_LIST})
    # base op files should be placed at ${_OP_NAME}/
    # base op main file should be name with "${_OP_NAME}_ref.c"
    file (GLOB _CPU_REF_REGISTER_FILE   "${_OP_ROOT}/${_OP_NAME}/${_OP_NAME}_ref.c")
    file (GLOB _CORTEX_A_REGISTER_FILE  "${_OP_ROOT}/${_OP_NAME}/cortex-a/*_hcl_arm.c")
    file (GLOB _CORTEX_M_REGISTER_FILE  "${_OP_ROOT}/${_OP_NAME}/cortex-m/*_cmsis.c")
    file (GLOB _x86_REGISTER_FILE       "${_OP_ROOT}/${_OP_NAME}/x86/*_hcl_x86.c")
    file (GLOB _MIPS_REGISTER_FILE      "${_OP_ROOT}/${_OP_NAME}/mips/*_hcl_mips.c")
    file (GLOB _RISC_V_REGISTER_FILE    "${_OP_ROOT}/${_OP_NAME}/risc-v/lp64dv/*_hcl_rv64.c")

    list (APPEND _CPU_REGISTER_SOURCE ${_CPU_REF_REGISTER_FILE})
    if (${TENGINE_TARGET_PROCESSOR} MATCHES "ARM")
        list (APPEND _CPU_REGISTER_SOURCE ${_CORTEX_A_REGISTER_FILE})
    endif()
    if (${TENGINE_TARGET_PROCESSOR} MATCHES "Cortex-M")
        list (APPEND _CPU_REGISTER_SOURCE ${_CORTEX_M_REGISTER_FILE})
    endif()
    if (${TENGINE_TARGET_PROCESSOR} MATCHES "X86")
        list (APPEND _CPU_REGISTER_SOURCE ${_x86_REGISTER_FILE})
    endif()
    if (${TENGINE_TARGET_PROCESSOR} MATCHES "MIPS")
        list (APPEND _CPU_REGISTER_SOURCE ${_MIPS_REGISTER_FILE})
    endif()
    if (${TENGINE_TARGET_PROCESSOR} MATCHES "lp64dv")
        list (APPEND _CPU_REGISTER_SOURCE ${_RISC_V_REGISTER_FILE})
    endif()
endforeach()


# 1.6 collect all source files to var
# 1.6.1 collect operator for Cortex-A source files
if (${TENGINE_TARGET_PROCESSOR} MATCHES "ARM")
    list (APPEND _CPU_HCL_SOURCE ${_CPU_CORTEX_A_SOURCE})
    tengine_source_group ("device/cpu/cortex-a" ${_CPU_CORTEX_A_SOURCE} )

    if (TENGINE_TARGET_PROCESSOR_32Bit)
        list (APPEND _CPU_HCL_SOURCE ${_CPU_CORTEX_A_V7_SOURCE})
        list (APPEND _CPU_HCL_SOURCE ${_CPU_CORTEX_A_V7_KERNEL})
        tengine_source_group ("device/cpu/cortex-a/armv7/source" ${_CPU_CORTEX_A_V7_SOURCE})
        tengine_source_group ("device/cpu/cortex-a/armv7/kernel" ${_CPU_CORTEX_A_V7_KERNEL})
    endif()
    if (TENGINE_TARGET_PROCESSOR_64Bit)
        list (APPEND _CPU_HCL_SOURCE ${_CPU_CORTEX_A_V8_SOURCE})
        list (APPEND _CPU_HCL_SOURCE ${_CPU_CORTEX_A_V8_KERNEL})
        tengine_source_group ("device/cpu/cortex-a/armv8/source" ${_CPU_CORTEX_A_V8_SOURCE})
        tengine_source_group ("device/cpu/cortex-a/armv8/kernel" ${_CPU_CORTEX_A_V8_KERNEL})

        if (TENGINE_ARCH_ARM_82)
            list (APPEND _CPU_HCL_SOURCE ${_CPU_CORTEX_A_V8_2_SOURCE})
            list (APPEND _CPU_HCL_SOURCE ${_CPU_CORTEX_A_V8_2_KERNEL})
            tengine_source_group ("device/cpu/cortex-a/armv8.2/source" ${_CPU_CORTEX_A_V8_2_SOURCE})
            tengine_source_group ("device/cpu/cortex-a/armv8.2/kernel" ${_CPU_CORTEX_A_V8_2_KERNEL})
        endif()
    endif()
endif()

# 1.6.2 collect operator for Cortex-M source files
if (${TENGINE_TARGET_PROCESSOR} MATCHES "Cortex-M")
    list (APPEND _CPU_HCL_SOURCE ${_CPU_CORTEX_M_SOURCE})
    list (APPEND _CPU_HCL_SOURCE ${_CPU_CORTEX_M_KERNEL})
    tengine_source_group ("device/cpu/cortex-m/source" ${_CPU_CORTEX_M_SOURCE})
    tengine_source_group ("device/cpu/cortex-m/kernel" ${_CPU_CORTEX_M_KERNEL})
endif()

# 1.6.3 collect operator for x86 source files
if (${TENGINE_TARGET_PROCESSOR} MATCHES "X86")
    list (APPEND _CPU_HCL_SOURCE ${_CPU_x86_SOURCE})
    list (APPEND _CPU_HCL_SOURCE ${_CPU_x86_KERNEL})
    tengine_source_group ("device/cpu/x86/source" ${_CPU_x86_SOURCE})
    tengine_source_group ("device/cpu/x86/kernel" ${_CPU_x86_KERNEL})
endif()

# 1.6.4 collect operator for x86 source files
if (${TENGINE_TARGET_PROCESSOR} MATCHES "MIPS")
    list (APPEND _CPU_HCL_SOURCE ${_CPU_MIPS_SOURCE})
    list (APPEND _CPU_HCL_SOURCE ${_CPU_MIPS_KERNEL})
    tengine_source_group ("device/cpu/mips/source" ${_CPU_MIPS_SOURCE})
    tengine_source_group ("device/cpu/mips/kernel" ${_CPU_MIPS_KERNEL})
endif()

# 1.6.5 collect operator for RISC-V lp64dv source files
if (${TENGINE_TARGET_PROCESSOR} MATCHES "lp64dv")
    list (APPEND _CPU_HCL_SOURCE ${_CPU_RISC_V_LP64DV_SOURCE})
    list (APPEND _CPU_HCL_SOURCE ${_CPU_RISC_V_LP64DV_KERNEL})
    tengine_source_group ("device/cpu/risc-v/source" ${_CPU_RISC_V_LP64DV_SOURCE})
    tengine_source_group ("device/cpu/risc-v/kernel" ${_CPU_RISC_V_LP64DV_KERNEL})
endif()


# 8. add build optioncds for cpu device
# 8.1 is a gcc or clang like compiler
if (TENGINE_COMPILER_GCC OR TENGINE_COMPILER_CLANG)
    if (${TENGINE_TARGET_PROCESSOR} MATCHES "ARM")
        if (TENGINE_TARGET_PROCESSOR_32Bit)
            if ((NOT (DEFINED TENGINE_TOOLCHAIN_FLAG)) AND (NOT ANDROID) AND (NOT CMAKE_CROSSCOMPILING) OR (CMAKE_CROSSCOMPILING AND OHOS))
                list (APPEND _CPU_COMPILER_OPTIONS "-march=armv7-a")
                list (APPEND _CPU_COMPILER_OPTIONS "-mfpu=neon")

                # need check float point when target process is aarch32
                include (${PROJECT_SOURCE_DIR}/cmake/environments/check_arm_hard_fp.cmake)
                tengine_check_arm32_hard_fp(_is_hard_float_point)

                if(${_is_hard_float_point})
                    list (APPEND _CPU_COMPILER_OPTIONS "-mfloat-abi=hard")
                else()
                    list (APPEND _CPU_COMPILER_OPTIONS "-mfloat-abi=softfp")
                endif()
            endif()

            if (NOT ANDROID AND NOT OHOS)
                list (APPEND _CPU_COMPILER_OPTIONS "-mfp16-format=ieee")
                #LIST (APPEND _CPU_COMPILER_OPTIONS "-mfpu=neon-fp16")
            endif()
        endif()

        if (TENGINE_TARGET_PROCESSOR_64BIT AND ${TENGINE_ARCH_ARM_82})
            list (APPEND _CPU_COMPILER_OPTIONS "-march=armv8.2-a+fp16")
        endif()
    endif()

    if (${TENGINE_TARGET_PROCESSOR} MATCHES "X86" AND ${TENGINE_ARCH_X86_AVX})
        list (APPEND _CPU_COMPILER_OPTIONS "-mfma")
        list (APPEND _CPU_COMPILER_OPTIONS "-mf16c")
    endif()

    if (${TENGINE_TARGET_PROCESSOR} MATCHES "MIPS")
        list (APPEND _CPU_COMPILER_OPTIONS "-mabi=64")
        list (APPEND _CPU_COMPILER_OPTIONS "-mmsa")

        list (APPEND _CPU_LINKER_OPTIONS   "-mabi=64")
        list (APPEND _CPU_LINKER_OPTIONS   "-mmsa")
    endif()

    if (${TENGINE_TARGET_PROCESSOR} MATCHES "lp64dv")
        list (APPEND _CPU_COMPILER_OPTIONS "-march=rv64gcvxthead")
        list (APPEND _CPU_COMPILER_OPTIONS "-mabi=lp64d")
        list (APPEND _CPU_COMPILER_OPTIONS "-mfp16")
        list (APPEND _CPU_COMPILER_OPTIONS "-lc")
    endif()    
endif()

# 8.2 is Microsoft Visual C++
if (TENGINE_COMPILER_MSVC)
    list (APPEND _CPU_COMPILER_DEFINES "NOMINMAX")
    list (APPEND _CPU_COMPILER_DEFINES "_SCL_SECURE_NO_WARNINGS")
    list (APPEND _CPU_COMPILER_DEFINES "_CRT_SECURE_NO_DEPRECATE")
    list (APPEND _CPU_COMPILER_DEFINES "_ENABLE_EXTENDED_ALIGNED_STORAGE")

    list (APPEND _CPU_COMPILER_OPTIONS "/MP")

    if (${TENGINE_TARGET_PROCESSOR} MATCHES "X86" AND ${TENGINE_ARCH_X86_AVX})
        list (APPEND _CPU_COMPILER_OPTIONS "/arch:AVX2")
    endif()
endif()


# 9. set all to cmake cache
set (TENGINE_CPU_HEADER_PATH        ${_DEV_CPU_HEADER_PATH}     CACHE INTERNAL  "Tengine CPU device header files searching path"    FORCE)
set (TENGINE_CPU_DEVICE_SOURCE      ${_CPU_DEVICE_SOURCE}       CACHE INTERNAL  "Tengine CPU device main source files"              FORCE)
set (TENGINE_CPU_REF_SOURCE         ${_CPU_REF_SOURCE}          CACHE INTERNAL  "Tengine CPU device reference op file"              FORCE)
set (TENGINE_CPU_HCL_SOURCE         ${_CPU_HCL_SOURCE}          CACHE INTERNAL  "Tengine CPU device high performance op file"       FORCE)
set (TENGINE_CPU_COMPILER_DEFINES   ${_CPU_COMPILER_DEFINES}    CACHE INTERNAL  "Tengine CPU about compiler defines"                FORCE)
set (TENGINE_CPU_COMPILER_OPTIONS   ${_CPU_COMPILER_OPTIONS}    CACHE INTERNAL  "Tengine CPU about compiler options"                FORCE)
set (TENGINE_CPU_LINKER_OPTIONS     ${_CPU_LINKER_OPTIONS}      CACHE INTERNAL  "Tengine CPU about linker options"                  FORCE)


# 10. module target: cpu device glue code, reference ops and high performance kernels
# in standalone HCL mode the hcl kernels are built into a separate library
set (_CPU_TARGET_SOURCE ${_CPU_DEVICE_SOURCE} ${_CPU_REF_SOURCE})
if (NOT TENGINE_STANDALONE_HCL)
    list (APPEND _CPU_TARGET_SOURCE ${_CPU_HCL_SOURCE})
endif()

add_library (tengine_device_cpu OBJECT ${_CPU_TARGET_SOURCE})
target_link_libraries (tengine_device_cpu PRIVATE tengine_build_config tengine_public_include)
target_include_directories (tengine_device_cpu PRIVATE ${_DEV_CPU_HEADER_PATH})
target_compile_definitions (tengine_device_cpu PRIVATE ${_CPU_COMPILER_DEFINES})
target_compile_options (tengine_device_cpu PRIVATE ${_CPU_COMPILER_OPTIONS})
