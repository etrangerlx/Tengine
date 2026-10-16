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
# Core library assembly.
#
# Architecture: every module under source/ builds its own OBJECT library
# (tengine_core_<name>, backends: tengine_device_<name>). Shared build
# configuration is carried by two INTERFACE targets:
#   - tengine_build_config   : common defines / compile options / link libs
#   - tengine_public_include : unified header search paths
# The final targets tengine-lite / tengine-lite-static aggregate all object
# libraries plus the source files of not-yet-migrated backend plugins.

# set target name
if (NOT DEFINED TENGINE_LITE_NAME)
    set (TENGINE_LITE_NAME ${CMAKE_PROJECT_NAME} CACHE INTERNAL "" FORCE)
endif()


# clear all legacy aggregation variables
unset (TENGINE_C_API_SOURCE)
unset (TENGINE_COMMON_DEFINES)
unset (TENGINE_COMMON_OPTIONS)
unset (TENGINE_COMMON_LINKER_OPTIONS)
unset (TENGINE_COMMON_LINK_LIBRARIES)
unset (TENGINE_SOURCE_FILES)
unset (TENGINE_HEADER_PATH)
unset (TENGINE_LINK_PATH)
unset (TENGINE_COMPILER_DEFINES)
unset (TENGINE_COMPILER_OPTIONS)
unset (TENGINE_LINKER_OPTIONS)
unset (TENGINE_LINK_LIBRARIES)
unset (TENGINE_ENABLED_OPENMP)
unset (TENGINE_OBJECT_TARGETS)


# ---------------------------------------------------------------------------
# common build configuration
# ---------------------------------------------------------------------------
if (TENGINE_SYSTEM_MACOS)
    list (APPEND TENGINE_COMMON_DEFINES "MACOS")
endif()

if (OHOS)
    list (APPEND TENGINE_COMMON_DEFINES "_OHOS_")

    # OHOS 3.0.0.80 patch for ASM language
    if (NOT DEFINED CMAKE_ASM_COMPILER_TARGET)
        set (CMAKE_ASM_COMPILER_TARGET ${OHOS_LLVM})
    endif()

    if (CMAKE_SYSTEM_NAME MATCHES "Windows")
        set (CMAKE_BUILD_WITH_INSTALL_RPATH ON)
    endif()
endif()

if (TENGINE_COMPILER_GCC OR TENGINE_COMPILER_CLANG)
    if (${TENGINE_FULL_CHECK})
        # warning local redefined same name variable
        list (APPEND TENGINE_COMMON_OPTIONS "-Wshadow")
        list (APPEND TENGINE_COMMON_OPTIONS "-Wunreachable-code")
        #LIST (APPEND TENGINE_COMMON_OPTIONS "-Werror")

        # show all warnings
        list (APPEND TENGINE_COMMON_OPTIONS "-Wall")

        # more warning details
        list (APPEND TENGINE_COMMON_OPTIONS "-Wextra")

        # warning all unused part
        list (APPEND TENGINE_COMMON_OPTIONS "-Wunknown-pragmas")
        list (APPEND TENGINE_COMMON_OPTIONS "-Wunused-label")
        list (APPEND TENGINE_COMMON_OPTIONS "-Wunused-value")
        list (APPEND TENGINE_COMMON_OPTIONS "-Wunused-function")
        list (APPEND TENGINE_COMMON_OPTIONS "-Wunused-variable")

        # check memory issues
        list (APPEND TENGINE_COMMON_OPTIONS "-fsanitize=address")
        list (APPEND TENGINE_COMMON_OPTIONS "-fno-omit-frame-pointer")

        list (APPEND TENGINE_COMMON_LINKER_OPTIONS "-fsanitize=address")
    endif()

    if ((${CMAKE_BUILD_TYPE} MATCHES "Debug") OR (${CMAKE_BUILD_TYPE} MATCHES "DEBUG"))
        list (APPEND TENGINE_COMMON_OPTIONS "-g")
        list (APPEND TENGINE_COMMON_OPTIONS "-O0")
    elseif (TENGINE_GENERATE_DEBUG_SYMBOL)
        list (APPEND TENGINE_COMMON_OPTIONS "-g")
    endif()

    if (${TENGINE_COVERAGE})
        list (APPEND TENGINE_COMMON_OPTIONS "-fprofile-arcs")
        list (APPEND TENGINE_COMMON_OPTIONS "-ftest-coverage")

        list (APPEND TENGINE_COMMON_LINK_LIBRARIES -coverage)
        list (APPEND TENGINE_COMMON_LINK_LIBRARIES -lgcov)
    endif()

    if (ANDROID)
        list (APPEND TENGINE_COMMON_LINK_LIBRARIES android)
        list (APPEND TENGINE_COMMON_LINK_LIBRARIES log)
    elseif (OHOS)
        list (APPEND TENGINE_COMMON_LINK_LIBRARIES dl)
        list (APPEND TENGINE_COMMON_LINK_LIBRARIES m)
    elseif (NOT MSVC)
        list (APPEND TENGINE_COMMON_LINK_LIBRARIES dl)
        list (APPEND TENGINE_COMMON_LINK_LIBRARIES m)
    endif()
endif()


# ---------------------------------------------------------------------------
# INTERFACE targets carrying shared build configuration
# ---------------------------------------------------------------------------
# tengine_build_config: common defines, compile and link options.
# modules link it PRIVATE, the final targets link it PRIVATE as well so the
# configuration never leaks into consumer build interfaces
add_library (tengine_build_config INTERFACE)
if (TENGINE_ONLINE_REPORT AND NOT MSVC)
    list (APPEND TENGINE_COMMON_DEFINES "TENGINE_ONLINE_REPORT=1")
endif()
target_compile_definitions (tengine_build_config INTERFACE ${TENGINE_COMMON_DEFINES})
target_compile_options (tengine_build_config INTERFACE ${TENGINE_COMMON_OPTIONS})
target_link_libraries (tengine_build_config INTERFACE ${TENGINE_COMMON_LINK_LIBRARIES})
target_link_options (tengine_build_config INTERFACE ${TENGINE_COMMON_LINKER_OPTIONS})

# tengine_public_include: unified header search paths.
# the visibility matches the pre-refactor TENGINE_HEADER_PATH union, so no
# existing include breaks; tighten per module later
add_library (tengine_public_include INTERFACE)
target_include_directories (tengine_public_include INTERFACE
    "${CMAKE_SOURCE_DIR}/source"
    "${CMAKE_BINARY_DIR}/source"
    "${CMAKE_SOURCE_DIR}/source/device"
    "${CMAKE_SOURCE_DIR}/source/device/cpu"
    "${CMAKE_SOURCE_DIR}/source/operator/prototype"
    "${CMAKE_SOURCE_DIR}/source/serializer"
    "${CMAKE_BINARY_DIR}/source/serializer"
)

# online report headers are used by the c api when the feature is enabled
if (TENGINE_ONLINE_REPORT AND NOT MSVC)
    target_include_directories (tengine_public_include INTERFACE "${CMAKE_SOURCE_DIR}/source/olreport/include")
endif()


# configure building
configure_file(defines.h.in ${CMAKE_CURRENT_BINARY_DIR}/defines.h)


# ---------------------------------------------------------------------------
# module targets with their own CMakeLists
# ---------------------------------------------------------------------------
add_subdirectory (device)                                   # each device as a executable backend
add_subdirectory (operator)                                 # operator framework and prototype
add_subdirectory (scheduler)                                # base and advanced scheduler for different backend
add_subdirectory (serializer)                               # multi-framework transform support

list (APPEND TENGINE_OBJECT_TARGETS
    tengine_core_device
    tengine_device_cpu
    tengine_core_operator
    tengine_core_scheduler
    tengine_core_serializer
)


# ---------------------------------------------------------------------------
# flat modules: object libraries defined here with explicit source lists
# ---------------------------------------------------------------------------
# api module: public C API, the entry of all user code
if (TENGINE_BUILD_CPP_API)
    file (GLOB _TENGINE_CXX_API_SOURCE CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/source/api/*.cc")
    if (NOT _TENGINE_CXX_API_SOURCE)
        message (FATAL_ERROR "TENGINE_BUILD_CPP_API is ON but no C++ API source found under source/api")
    endif()
    set (_TENGINE_API_SOURCE ${_TENGINE_CXX_API_SOURCE})
else()
    set (_TENGINE_API_SOURCE
        "${CMAKE_SOURCE_DIR}/source/api/c_api.c"
        "${CMAKE_SOURCE_DIR}/source/api/plugin.c"
    )
endif()

add_library (tengine_core_api OBJECT ${_TENGINE_API_SOURCE})
target_link_libraries (tengine_core_api PRIVATE tengine_build_config tengine_public_include)
list (APPEND TENGINE_OBJECT_TARGETS tengine_core_api)


# executer module: layer by layer graph executor
add_library (tengine_core_executer OBJECT
    "${CMAKE_SOURCE_DIR}/source/executer/executer.c"
)
target_link_libraries (tengine_core_executer PRIVATE tengine_build_config tengine_public_include)
list (APPEND TENGINE_OBJECT_TARGETS tengine_core_executer)


# graph module: ir graph, node, tensor and subgraph
add_library (tengine_core_graph OBJECT
    "${CMAKE_SOURCE_DIR}/source/graph/graph.c"
    "${CMAKE_SOURCE_DIR}/source/graph/node.c"
    "${CMAKE_SOURCE_DIR}/source/graph/subgraph.c"
    "${CMAKE_SOURCE_DIR}/source/graph/tensor.c"
)
target_link_libraries (tengine_core_graph PRIVATE tengine_build_config tengine_public_include)
list (APPEND TENGINE_OBJECT_TARGETS tengine_core_graph)


# module module: dynamic module registration
add_library (tengine_core_module OBJECT
    "${CMAKE_SOURCE_DIR}/source/module/module.c"
)
target_link_libraries (tengine_core_module PRIVATE tengine_build_config tengine_public_include)
list (APPEND TENGINE_OBJECT_TARGETS tengine_core_module)


# optimizer module: graph partition, estimation and helper
add_library (tengine_core_optimizer OBJECT
    "${CMAKE_SOURCE_DIR}/source/optimizer/estimation.c"
    "${CMAKE_SOURCE_DIR}/source/optimizer/helper.c"
    "${CMAKE_SOURCE_DIR}/source/optimizer/split.c"
)
target_link_libraries (tengine_core_optimizer PRIVATE tengine_build_config tengine_public_include)
list (APPEND TENGINE_OBJECT_TARGETS tengine_core_optimizer)


# system module: cpu affinity and system info
add_library (tengine_core_system OBJECT
    "${CMAKE_SOURCE_DIR}/source/system/cpu.c"
)
target_link_libraries (tengine_core_system PRIVATE tengine_build_config tengine_public_include)
list (APPEND TENGINE_OBJECT_TARGETS tengine_core_system)


# utility module: log, memory, math, vector and other basic facilities
add_library (tengine_core_utility OBJECT
    "${CMAKE_SOURCE_DIR}/source/utility/float.c"
    "${CMAKE_SOURCE_DIR}/source/utility/lock.c"
    "${CMAKE_SOURCE_DIR}/source/utility/log.c"
    "${CMAKE_SOURCE_DIR}/source/utility/math.c"
    "${CMAKE_SOURCE_DIR}/source/utility/mem_stat.c"
    "${CMAKE_SOURCE_DIR}/source/utility/sys_port.c"
    "${CMAKE_SOURCE_DIR}/source/utility/utils.c"
    "${CMAKE_SOURCE_DIR}/source/utility/vector.c"
)
target_link_libraries (tengine_core_utility PRIVATE tengine_build_config tengine_public_include)
list (APPEND TENGINE_OBJECT_TARGETS tengine_core_utility)


# online report module: optional, non MSVC only
if (NOT MSVC)
    if (TENGINE_ONLINE_REPORT)
        file (GLOB_RECURSE TENGINE_ONLINE_REPORT_SRCS CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/source/olreport/*.c")
        add_library (tengine_core_olreport OBJECT ${TENGINE_ONLINE_REPORT_SRCS})
        target_link_libraries (tengine_core_olreport PRIVATE tengine_build_config tengine_public_include)
        target_include_directories (tengine_core_olreport PRIVATE "${CMAKE_SOURCE_DIR}/source/olreport/include")
        list (APPEND TENGINE_OBJECT_TARGETS tengine_core_olreport)
    endif()
endif()


# ---------------------------------------------------------------------------
# legacy header path / aggregation variables (kept for backend plugins)
# ---------------------------------------------------------------------------
# add header file searching path
list (APPEND TENGINE_HEADER_PATH        "${CMAKE_SOURCE_DIR}/source")
list (APPEND TENGINE_HEADER_PATH        "${CMAKE_BINARY_DIR}/source")
list (APPEND TENGINE_HEADER_PATH        ${TENGINE_DEVICE_HEADER_PATH})
list (APPEND TENGINE_HEADER_PATH        ${TENGINE_OP_HEADER_PATH})
list (APPEND TENGINE_HEADER_PATH        ${TENGINE_SERIALIZER_HEADER_PATH})


# link libraries search path
list (APPEND TENGINE_LINK_PATH          ${TENGINE_DEVICE_LINK_PATH})


# add compiler defines
list (APPEND TENGINE_COMPILER_DEFINES   ${TENGINE_COMMON_DEFINES})
list (APPEND TENGINE_COMPILER_DEFINES   ${TENGINE_DEVICE_COMPILER_DEFINES})
list (APPEND TENGINE_COMPILER_DEFINES   ${TENGINE_OPERATOR_COMPILER_DEFINES})
list (APPEND TENGINE_COMPILER_DEFINES   ${TENGINE_SCHEDULER_COMPILER_DEFINES})
list (APPEND TENGINE_COMPILER_DEFINES   ${TENGINE_SERIALIZER_COMPILER_DEFINES})
list (APPEND TENGINE_COMPILER_DEFINES   ${TENGINE_EXECUTER_COMPILER_DEFINES})
list (APPEND TENGINE_COMPILER_DEFINES   ${TENGINE_EXPORTER_COMPILER_DEFINES})
list (APPEND TENGINE_COMPILER_DEFINES   ${TENGINE_OPTIMIZER_COMPILER_DEFINES})


# add compiler options
list (APPEND TENGINE_COMPILER_OPTIONS   ${TENGINE_COMMON_OPTIONS})
list (APPEND TENGINE_COMPILER_OPTIONS   ${TENGINE_DEVICE_COMPILER_OPTIONS})
list (APPEND TENGINE_COMPILER_OPTIONS   ${TENGINE_OPERATOR_COMPILER_OPTIONS})
list (APPEND TENGINE_COMPILER_OPTIONS   ${TENGINE_SCHEDULER_COMPILER_OPTIONS})
list (APPEND TENGINE_COMPILER_OPTIONS   ${TENGINE_SERIALIZER_COMPILER_OPTIONS})
list (APPEND TENGINE_COMPILER_OPTIONS   ${TENGINE_EXECUTER_COMPILER_OPTIONS})
list (APPEND TENGINE_COMPILER_OPTIONS   ${TENGINE_EXPORTER_COMPILER_OPTIONS})
list (APPEND TENGINE_COMPILER_OPTIONS   ${TENGINE_GRAPH_COMPILER_OPTIONS})
list (APPEND TENGINE_COMPILER_OPTIONS   ${TENGINE_MODULE_COMPILER_OPTIONS})
list (APPEND TENGINE_COMPILER_OPTIONS   ${TENGINE_OPTIMIZER_COMPILER_OPTIONS})
list (APPEND TENGINE_COMPILER_OPTIONS   ${TENGINE_UTILITY_COMPILER_OPTIONS})


# add linker options
list (APPEND TENGINE_LINKER_OPTIONS     ${TENGINE_COMMON_LINKER_OPTIONS})
list (APPEND TENGINE_LINKER_OPTIONS     ${TENGINE_DEVICE_LINKER_OPTIONS})
list (APPEND TENGINE_LINKER_OPTIONS     ${TENGINE_OPERATOR_LINKER_OPTIONS})
list (APPEND TENGINE_LINKER_OPTIONS     ${TENGINE_SCHEDULER_LINKER_OPTIONS})
list (APPEND TENGINE_LINKER_OPTIONS     ${TENGINE_SERIALIZER_LINKER_OPTIONS})
list (APPEND TENGINE_LINKER_OPTIONS     ${TENGINE_EXECUTER_LINKER_OPTIONS})
list (APPEND TENGINE_LINKER_OPTIONS     ${TENGINE_EXPORTER_LINKER_OPTIONS})
list (APPEND TENGINE_LINKER_OPTIONS     ${TENGINE_GRAPH_LINKER_OPTIONS})
list (APPEND TENGINE_LINKER_OPTIONS     ${TENGINE_MODULE_LINKER_OPTIONS})
list (APPEND TENGINE_LINKER_OPTIONS     ${TENGINE_OPTIMIZER_LINKER_OPTIONS})
list (APPEND TENGINE_LINKER_OPTIONS     ${TENGINE_UTILITY_LINKER_OPTIONS})


# add link libraries
list (APPEND TENGINE_LINK_LIBRARIES     ${TENGINE_COMMON_LINK_LIBRARIES})
list (APPEND TENGINE_LINK_LIBRARIES     ${TENGINE_DEVICE_LINK_LIBRARIES})
list (APPEND TENGINE_LINK_LIBRARIES     ${TENGINE_OPERATOR_LINK_LIBRARIES})
list (APPEND TENGINE_LINK_LIBRARIES     ${TENGINE_SCHEDULER_LINK_LIBRARIES})
list (APPEND TENGINE_LINK_LIBRARIES     ${TENGINE_SERIALIZER_LINK_LIBRARIES})
list (APPEND TENGINE_LINK_LIBRARIES     ${TENGINE_EXECUTER_LINK_LIBRARIES})
list (APPEND TENGINE_LINK_LIBRARIES     ${TENGINE_EXPORTER_LINK_LIBRARIES})
list (APPEND TENGINE_LINK_LIBRARIES     ${TENGINE_GRAPH_LINK_LIBRARIES})
list (APPEND TENGINE_LINK_LIBRARIES     ${TENGINE_MODULE_LINK_LIBRARIES})
list (APPEND TENGINE_LINK_LIBRARIES     ${TENGINE_OPTIMIZER_LINK_LIBRARIES})
list (APPEND TENGINE_LINK_LIBRARIES     ${TENGINE_UTILITY_LINK_LIBRARIES})


# backend plugin source files, compiled directly into the final targets until
# each backend grows its own object library
list (APPEND TENGINE_SOURCE_FILES       ${TENGINE_DEVICE_SOURCE})


# ---------------------------------------------------------------------------
# final targets: static and shared library
# ---------------------------------------------------------------------------
# export the module list to parent scope, cmake/summary.cmake prints it
set (TENGINE_OBJECT_TARGETS ${TENGINE_OBJECT_TARGETS} CACHE INTERNAL "Tengine module object targets" FORCE)

add_library (${TENGINE_LITE_NAME}-static STATIC)
add_library (${TENGINE_LITE_NAME}        SHARED)

# add module object files to each target
foreach (_MODULE_TARGET ${TENGINE_OBJECT_TARGETS})
    target_sources (${TENGINE_LITE_NAME}-static PRIVATE $<TARGET_OBJECTS:${_MODULE_TARGET}>)
    target_sources (${TENGINE_LITE_NAME}        PRIVATE $<TARGET_OBJECTS:${_MODULE_TARGET}>)
endforeach()

# add backend plugin source files to each target
target_sources (${TENGINE_LITE_NAME}-static PRIVATE ${TENGINE_SOURCE_FILES})
target_sources (${TENGINE_LITE_NAME}        PRIVATE ${TENGINE_SOURCE_FILES})

# add header file search path
target_include_directories (${TENGINE_LITE_NAME}-static PRIVATE ${TENGINE_HEADER_PATH})
target_include_directories (${TENGINE_LITE_NAME}        PRIVATE ${TENGINE_HEADER_PATH})

# common build configuration, private so it never leaks to consumers
target_link_libraries (${TENGINE_LITE_NAME}-static PRIVATE tengine_build_config)
target_link_libraries (${TENGINE_LITE_NAME}        PRIVATE tengine_build_config)

# add link libraries search path
target_link_directories (${TENGINE_LITE_NAME}-static PUBLIC ${TENGINE_LINK_PATH})
target_link_directories (${TENGINE_LITE_NAME}        PUBLIC ${TENGINE_LINK_PATH})

# add defines
target_compile_definitions (${TENGINE_LITE_NAME}-static PRIVATE $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:${TENGINE_COMPILER_DEFINES}>)
target_compile_definitions (${TENGINE_LITE_NAME}        PRIVATE $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:${TENGINE_COMPILER_DEFINES}>)

# add options
target_compile_options (${TENGINE_LITE_NAME}-static PUBLIC $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:${TENGINE_COMPILER_OPTIONS}>)
target_compile_options (${TENGINE_LITE_NAME}        PUBLIC $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:${TENGINE_COMPILER_OPTIONS}>)

# add link options (target_LINKER_OPTIONS need CMake 3.13 at least)
target_link_options (${TENGINE_LITE_NAME}-static PUBLIC $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:${TENGINE_LINKER_OPTIONS}>)
target_link_options (${TENGINE_LITE_NAME}        PUBLIC $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:${TENGINE_LINKER_OPTIONS}>)

# add link libraries
target_link_libraries (${TENGINE_LITE_NAME}-static PUBLIC  ${TENGINE_LINK_LIBRARIES})
target_link_libraries (${TENGINE_LITE_NAME}        PUBLIC  ${TENGINE_LINK_LIBRARIES})


# ---------------------------------------------------------------------------
# toolchain and OpenMP flags for module object libraries
# ---------------------------------------------------------------------------
# openmp detection first, object libraries need the compile flags as well,
# otherwise #pragma omp would be silently ignored
unset (_has_openmp)
if (NOT (TENGINE_FORCE_SKIP_OPENMP) AND TENGINE_OPENMP)
    include (${PROJECT_SOURCE_DIR}/cmake/libraries/openmp.cmake)
    tengine_check_lib_openmp (_has_openmp)
endif()

foreach (_MODULE_TARGET ${TENGINE_OBJECT_TARGETS})
    # objects are linked into both the static and the shared library,
    # position independent code is required by the shared one
    set_target_properties (${_MODULE_TARGET} PROPERTIES POSITION_INDEPENDENT_CODE ON)

    tengine_apply_toolchain_flags (${_MODULE_TARGET})
    if (_has_openmp)
        tengine_compile_lib_openmp (${_MODULE_TARGET})
    endif()

    # strip unused sections, helps reducing the size of the static library
    if (TENGINE_COMPILER_GCC OR TENGINE_COMPILER_CLANG)
        target_compile_options (${_MODULE_TARGET} PRIVATE $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:-fdata-sections>)
        target_compile_options (${_MODULE_TARGET} PRIVATE $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:-ffunction-sections>)

        # hide internal symbols in the shared library, the public C API stays
        # visible through the DLLEXPORT attribute in c_api.h
        if (NOT TENGINE_ENABLE_ALL_SYMBOL)
            target_compile_options (${_MODULE_TARGET} PRIVATE $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:-fvisibility=hidden>)
        endif()
    endif()
endforeach()


# default support openmp
if (_has_openmp)
    tengine_use_lib_openmp (${TENGINE_LITE_NAME}-static ON)
    tengine_use_lib_openmp (${TENGINE_LITE_NAME}        OFF)
    if(MSVC)
        set_source_files_properties(${TENGINE_SOURCE_FILES} PROPERTIES LANGUAGE CXX)
        target_compile_options(${TENGINE_LITE_NAME}-static PRIVATE /std:c++latest)
        target_compile_options(${TENGINE_LITE_NAME}        PRIVATE /std:c++latest)
    endif()

    set (TENGINE_ENABLED_OPENMP ${OpenMP_C_VERSION} CACHE INTERNAL "" FORCE)
endif()
unset (_has_openmp)

# pthread
if (NOT OHOS)
	include (${PROJECT_SOURCE_DIR}/cmake/libraries/pthread.cmake)
	tengine_check_lib_pthread (_has_pthread)
	if (_has_pthread)
		tengine_use_lib_pthread (${TENGINE_LITE_NAME}-static ON)
		tengine_use_lib_pthread (${TENGINE_LITE_NAME}        OFF)
	endif()
	unset (_has_pthread)
endif()


#
if (TENGINE_COMPILER_GCC OR TENGINE_COMPILER_CLANG)
    # linking time optimization
    if ((NOT ANDROID) AND (NOT OHOS))
        #TARGET_COMPILE_OPTIONS (${TENGINE_LITE_NAME} PRIVATE $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:-flto>)
    endif()

    if (NOT TENGINE_ENABLE_ALL_SYMBOL)
        target_compile_options (${TENGINE_LITE_NAME} PRIVATE $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:-fvisibility=hidden>)
    endif()

    # it seems will lead to some error
    #ADD_COMPILE_OPTIONS ($<$<COMPILE_LANGUAGE:CXX>:-Wl,-rpath,'$ORIGIN:/lib>)

    #SET_TARGET_PROPERTIES(${TENGINE_LITE_NAME}-static PROPERTIES LINK_FLAGS "-Wl,--exclude-libs,ALL")
endif()


# apply external toolchain flags to the final targets
tengine_apply_toolchain_flags (${TENGINE_LITE_NAME}-static)
tengine_apply_toolchain_flags (${TENGINE_LITE_NAME})


# cuda above option
if (TENGINE_ENABLE_CUDA)
    if (TENGINE_ENV_HAS_CXX14)
        set_property(TARGET ${TENGINE_LITE_NAME}-static PROPERTY CUDA_STANDARD 14)
        set_property(TARGET ${TENGINE_LITE_NAME}        PROPERTY CUDA_STANDARD 14)
    else()
        set_property(TARGET ${TENGINE_LITE_NAME}-static PROPERTY CUDA_STANDARD 11)
        set_property(TARGET ${TENGINE_LITE_NAME}        PROPERTY CUDA_STANDARD 11)
    endif()
    set_property(TARGET ${TENGINE_LITE_NAME}-static PROPERTY CUDA_STANDARD_REQUIRED ON)
    set_property(TARGET ${TENGINE_LITE_NAME}        PROPERTY CUDA_STANDARD_REQUIRED ON)

    target_compile_options (${TENGINE_LITE_NAME}-static PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-Werror=cross-execution-space-call>)
    target_compile_options (${TENGINE_LITE_NAME}        PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-Werror=cross-execution-space-call>)

    if (${CMAKE_MINOR_VERSION} GREATER 17)
        if (TENGINE_TARGET_PROCESSOR MATCHES "ARM")
            set_property(TARGET ${TENGINE_LITE_NAME}-static PROPERTY CUDA_ARCHITECTURES 53 62 72)
            set_property(TARGET ${TENGINE_LITE_NAME}        PROPERTY CUDA_ARCHITECTURES 53 62 72)
        else()
            set_property(TARGET ${TENGINE_LITE_NAME}-static PROPERTY CUDA_ARCHITECTURES 50 52 61 70 75)
            set_property(TARGET ${TENGINE_LITE_NAME}        PROPERTY CUDA_ARCHITECTURES 50 52 61 70 75)
        endif()
    endif()
endif()

# deal with depends
foreach(_var ${TENGINE_DEVICE_DEPENDS_FORWARD})
    add_dependencies(${TENGINE_LITE_NAME}-static ${_var})
    add_dependencies(${TENGINE_LITE_NAME}        ${_var})
endforeach()

# debug macro information, applied on module object libraries so both the
# static and shared library behave the same (all switches default to OFF)
set (_TENGINE_DEBUG_DEFINES)
if (TENGINE_DEBUG_MEM_STAT)
    list (APPEND _TENGINE_DEBUG_DEFINES CONFIG_MEM_STAT)
endif()
if (TENGINE_DEBUG_DATA)
    list (APPEND _TENGINE_DEBUG_DEFINES DEBUG_DATA)
endif()
if (TENGINE_DEBUG_TIME)
    list (APPEND _TENGINE_DEBUG_DEFINES DEBUG_TIME)
endif()
if (TENGINE_LITE_VERSION)
    list (APPEND _TENGINE_DEBUG_DEFINES TENGINE_LITE_VERSION=${TENGINE_LITE_VERSION})
endif()
if (TENGINE_VERSION_POSTFIX)
    list (APPEND _TENGINE_DEBUG_DEFINES TENGINE_VERSION_POSTFIX=${TENGINE_VERSION_POSTFIX})
endif()

if (_TENGINE_DEBUG_DEFINES)
    foreach (_MODULE_TARGET ${TENGINE_OBJECT_TARGETS})
        target_compile_definitions(${_MODULE_TARGET} PRIVATE ${_TENGINE_DEBUG_DEFINES})
    endforeach()

    # backend plugin sources are compiled inside the final targets
    target_compile_definitions(${TENGINE_LITE_NAME}-static PRIVATE $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:${_TENGINE_DEBUG_DEFINES}>)
    target_compile_definitions(${TENGINE_LITE_NAME}        PRIVATE $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:${_TENGINE_DEBUG_DEFINES}>)
endif()
unset (_TENGINE_DEBUG_DEFINES)

set_property(TARGET ${CMAKE_PROJECT_NAME}-static PROPERTY FOLDER "libtengine-lite")
set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY FOLDER "libtengine-lite")

# install tengine libraries
install (TARGETS ${CMAKE_PROJECT_NAME}-static DESTINATION lib)
install (TARGETS ${CMAKE_PROJECT_NAME}        DESTINATION lib)

if (TENGINE_STANDALONE_HCL)
    install (TARGETS ${TENGINE_STANDALONE_HCL_LIB_NAME} DESTINATION lib)
endif()

# install header files
install (FILES ${CMAKE_SOURCE_DIR}/source/api/c_api.h DESTINATION include/tengine)
install (FILES ${CMAKE_CURRENT_BINARY_DIR}/defines.h  DESTINATION include/tengine)

if (TENGINE_BUILD_CPP_API)
    install (FILES ${CMAKE_SOURCE_DIR}/source/api/cpp_api.h DESTINATION include/tengine)
endif()
