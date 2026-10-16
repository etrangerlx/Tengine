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
IF (NOT DEFINED TENGINE_LITE_NAME)
    SET (TENGINE_LITE_NAME ${CMAKE_PROJECT_NAME} CACHE INTERNAL "" FORCE)
ENDIF()


# clear all legacy aggregation variables
UNSET (TENGINE_C_API_SOURCE)
UNSET (TENGINE_COMMON_DEFINES)
UNSET (TENGINE_COMMON_OPTIONS)
UNSET (TENGINE_COMMON_LINKER_OPTIONS)
UNSET (TENGINE_COMMON_LINK_LIBRARIES)
UNSET (TENGINE_SOURCE_FILES)
UNSET (TENGINE_HEADER_PATH)
UNSET (TENGINE_LINK_PATH)
UNSET (TENGINE_COMPILER_DEFINES)
UNSET (TENGINE_COMPILER_OPTIONS)
UNSET (TENGINE_LINKER_OPTIONS)
UNSET (TENGINE_LINK_LIBRARIES)
UNSET (TENGINE_ENABLED_OPENMP)
UNSET (TENGINE_OBJECT_TARGETS)


# ---------------------------------------------------------------------------
# common build configuration
# ---------------------------------------------------------------------------
IF (TENGINE_SYSTEM_MACOS)
    LIST (APPEND TENGINE_COMMON_DEFINES "MACOS")
ENDIF()

IF (OHOS)
    LIST (APPEND TENGINE_COMMON_DEFINES "_OHOS_")

    # OHOS 3.0.0.80 patch for ASM language
    IF (NOT DEFINED CMAKE_ASM_COMPILER_TARGET)
        SET (CMAKE_ASM_COMPILER_TARGET ${OHOS_LLVM})
    ENDIF()

    IF (CMAKE_SYSTEM_NAME MATCHES "Windows")
        SET (CMAKE_BUILD_WITH_INSTALL_RPATH ON)
    ENDIF()
ENDIF()

IF (TENGINE_COMPILER_GCC OR TENGINE_COMPILER_CLANG)
    IF (${TENGINE_FULL_CHECK})
        # warning local redefined same name variable
        LIST (APPEND TENGINE_COMMON_OPTIONS "-Wshadow")
        LIST (APPEND TENGINE_COMMON_OPTIONS "-Wunreachable-code")
        #LIST (APPEND TENGINE_COMMON_OPTIONS "-Werror")

        # show all warnings
        LIST (APPEND TENGINE_COMMON_OPTIONS "-Wall")

        # more warning details
        LIST (APPEND TENGINE_COMMON_OPTIONS "-Wextra")

        # warning all unused part
        LIST (APPEND TENGINE_COMMON_OPTIONS "-Wunknown-pragmas")
        LIST (APPEND TENGINE_COMMON_OPTIONS "-Wunused-label")
        LIST (APPEND TENGINE_COMMON_OPTIONS "-Wunused-value")
        LIST (APPEND TENGINE_COMMON_OPTIONS "-Wunused-function")
        LIST (APPEND TENGINE_COMMON_OPTIONS "-Wunused-variable")

        # check memory issues
        LIST (APPEND TENGINE_COMMON_OPTIONS "-fsanitize=address")
        LIST (APPEND TENGINE_COMMON_OPTIONS "-fno-omit-frame-pointer")

        LIST (APPEND TENGINE_COMMON_LINKER_OPTIONS "-fsanitize=address")
    ENDIF()

    IF ((${CMAKE_BUILD_TYPE} MATCHES "Debug") OR (${CMAKE_BUILD_TYPE} MATCHES "DEBUG"))
        LIST (APPEND TENGINE_COMMON_OPTIONS "-g")
        LIST (APPEND TENGINE_COMMON_OPTIONS "-O0")
    ELSEIF (TENGINE_GENERATE_DEBUG_SYMBOL)
        LIST (APPEND TENGINE_COMMON_OPTIONS "-g")
    ENDIF()

    IF (${TENGINE_COVERAGE})
        LIST (APPEND TENGINE_COMMON_OPTIONS "-fprofile-arcs")
        LIST (APPEND TENGINE_COMMON_OPTIONS "-ftest-coverage")

        LIST (APPEND TENGINE_COMMON_LINK_LIBRARIES -coverage)
        LIST (APPEND TENGINE_COMMON_LINK_LIBRARIES -lgcov)
    ENDIF()

    IF (ANDROID)
        LIST (APPEND TENGINE_COMMON_LINK_LIBRARIES android)
        LIST (APPEND TENGINE_COMMON_LINK_LIBRARIES log)
    ELSEIF (OHOS)
        LIST (APPEND TENGINE_COMMON_LINK_LIBRARIES dl)
        LIST (APPEND TENGINE_COMMON_LINK_LIBRARIES m)
    ELSEIF (NOT MSVC)
        LIST (APPEND TENGINE_COMMON_LINK_LIBRARIES dl)
        LIST (APPEND TENGINE_COMMON_LINK_LIBRARIES m)
    ENDIF()
ENDIF()


# ---------------------------------------------------------------------------
# INTERFACE targets carrying shared build configuration
# ---------------------------------------------------------------------------
# tengine_build_config: common defines, compile and link options.
# modules link it PRIVATE, the final targets link it PRIVATE as well so the
# configuration never leaks into consumer build interfaces
ADD_LIBRARY (tengine_build_config INTERFACE)
IF (TENGINE_ONLINE_REPORT AND NOT MSVC)
    LIST (APPEND TENGINE_COMMON_DEFINES "TENGINE_ONLINE_REPORT=1")
ENDIF()
TARGET_COMPILE_DEFINITIONS (tengine_build_config INTERFACE ${TENGINE_COMMON_DEFINES})
TARGET_COMPILE_OPTIONS (tengine_build_config INTERFACE ${TENGINE_COMMON_OPTIONS})
TARGET_LINK_LIBRARIES (tengine_build_config INTERFACE ${TENGINE_COMMON_LINK_LIBRARIES})
TARGET_LINK_OPTIONS (tengine_build_config INTERFACE ${TENGINE_COMMON_LINKER_OPTIONS})

# tengine_public_include: unified header search paths.
# the visibility matches the pre-refactor TENGINE_HEADER_PATH union, so no
# existing include breaks; tighten per module later
ADD_LIBRARY (tengine_public_include INTERFACE)
TARGET_INCLUDE_DIRECTORIES (tengine_public_include INTERFACE
    "${CMAKE_SOURCE_DIR}/source"
    "${CMAKE_BINARY_DIR}/source"
    "${CMAKE_SOURCE_DIR}/source/device"
    "${CMAKE_SOURCE_DIR}/source/device/cpu"
    "${CMAKE_SOURCE_DIR}/source/operator/prototype"
    "${CMAKE_SOURCE_DIR}/source/serializer"
    "${CMAKE_BINARY_DIR}/source/serializer"
)

# online report headers are used by the c api when the feature is enabled
IF (TENGINE_ONLINE_REPORT AND NOT MSVC)
    TARGET_INCLUDE_DIRECTORIES (tengine_public_include INTERFACE "${CMAKE_SOURCE_DIR}/source/olreport/include")
ENDIF()


# configure building
CONFIGURE_FILE(defines.h.in ${CMAKE_CURRENT_BINARY_DIR}/defines.h)


# ---------------------------------------------------------------------------
# module targets with their own CMakeLists
# ---------------------------------------------------------------------------
ADD_SUBDIRECTORY (device)                                   # each device as a executable backend
ADD_SUBDIRECTORY (operator)                                 # operator framework and prototype
ADD_SUBDIRECTORY (scheduler)                                # base and advanced scheduler for different backend
ADD_SUBDIRECTORY (serializer)                               # multi-framework transform support

LIST (APPEND TENGINE_OBJECT_TARGETS
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
IF (TENGINE_BUILD_CPP_API)
    FILE (GLOB _TENGINE_CXX_API_SOURCE CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/source/api/*.cc")
    IF (NOT _TENGINE_CXX_API_SOURCE)
        MESSAGE (FATAL_ERROR "TENGINE_BUILD_CPP_API is ON but no C++ API source found under source/api")
    ENDIF()
    SET (_TENGINE_API_SOURCE ${_TENGINE_CXX_API_SOURCE})
ELSE()
    SET (_TENGINE_API_SOURCE
        "${CMAKE_SOURCE_DIR}/source/api/c_api.c"
        "${CMAKE_SOURCE_DIR}/source/api/plugin.c"
    )
ENDIF()

ADD_LIBRARY (tengine_core_api OBJECT ${_TENGINE_API_SOURCE})
TARGET_LINK_LIBRARIES (tengine_core_api PRIVATE tengine_build_config tengine_public_include)
LIST (APPEND TENGINE_OBJECT_TARGETS tengine_core_api)


# executer module: layer by layer graph executor
ADD_LIBRARY (tengine_core_executer OBJECT
    "${CMAKE_SOURCE_DIR}/source/executer/executer.c"
)
TARGET_LINK_LIBRARIES (tengine_core_executer PRIVATE tengine_build_config tengine_public_include)
LIST (APPEND TENGINE_OBJECT_TARGETS tengine_core_executer)


# graph module: ir graph, node, tensor and subgraph
ADD_LIBRARY (tengine_core_graph OBJECT
    "${CMAKE_SOURCE_DIR}/source/graph/graph.c"
    "${CMAKE_SOURCE_DIR}/source/graph/node.c"
    "${CMAKE_SOURCE_DIR}/source/graph/subgraph.c"
    "${CMAKE_SOURCE_DIR}/source/graph/tensor.c"
)
TARGET_LINK_LIBRARIES (tengine_core_graph PRIVATE tengine_build_config tengine_public_include)
LIST (APPEND TENGINE_OBJECT_TARGETS tengine_core_graph)


# module module: dynamic module registration
ADD_LIBRARY (tengine_core_module OBJECT
    "${CMAKE_SOURCE_DIR}/source/module/module.c"
)
TARGET_LINK_LIBRARIES (tengine_core_module PRIVATE tengine_build_config tengine_public_include)
LIST (APPEND TENGINE_OBJECT_TARGETS tengine_core_module)


# optimizer module: graph partition, estimation and helper
ADD_LIBRARY (tengine_core_optimizer OBJECT
    "${CMAKE_SOURCE_DIR}/source/optimizer/estimation.c"
    "${CMAKE_SOURCE_DIR}/source/optimizer/helper.c"
    "${CMAKE_SOURCE_DIR}/source/optimizer/split.c"
)
TARGET_LINK_LIBRARIES (tengine_core_optimizer PRIVATE tengine_build_config tengine_public_include)
LIST (APPEND TENGINE_OBJECT_TARGETS tengine_core_optimizer)


# system module: cpu affinity and system info
ADD_LIBRARY (tengine_core_system OBJECT
    "${CMAKE_SOURCE_DIR}/source/system/cpu.c"
)
TARGET_LINK_LIBRARIES (tengine_core_system PRIVATE tengine_build_config tengine_public_include)
LIST (APPEND TENGINE_OBJECT_TARGETS tengine_core_system)


# utility module: log, memory, math, vector and other basic facilities
ADD_LIBRARY (tengine_core_utility OBJECT
    "${CMAKE_SOURCE_DIR}/source/utility/float.c"
    "${CMAKE_SOURCE_DIR}/source/utility/lock.c"
    "${CMAKE_SOURCE_DIR}/source/utility/log.c"
    "${CMAKE_SOURCE_DIR}/source/utility/math.c"
    "${CMAKE_SOURCE_DIR}/source/utility/mem_stat.c"
    "${CMAKE_SOURCE_DIR}/source/utility/sys_port.c"
    "${CMAKE_SOURCE_DIR}/source/utility/utils.c"
    "${CMAKE_SOURCE_DIR}/source/utility/vector.c"
)
TARGET_LINK_LIBRARIES (tengine_core_utility PRIVATE tengine_build_config tengine_public_include)
LIST (APPEND TENGINE_OBJECT_TARGETS tengine_core_utility)


# online report module: optional, non MSVC only
IF (NOT MSVC)
    IF (TENGINE_ONLINE_REPORT)
        FILE (GLOB_RECURSE TENGINE_ONLINE_REPORT_SRCS CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/source/olreport/*.c")
        ADD_LIBRARY (tengine_core_olreport OBJECT ${TENGINE_ONLINE_REPORT_SRCS})
        TARGET_LINK_LIBRARIES (tengine_core_olreport PRIVATE tengine_build_config tengine_public_include)
        TARGET_INCLUDE_DIRECTORIES (tengine_core_olreport PRIVATE "${CMAKE_SOURCE_DIR}/source/olreport/include")
        LIST (APPEND TENGINE_OBJECT_TARGETS tengine_core_olreport)
    ENDIF()
ENDIF()


# ---------------------------------------------------------------------------
# legacy header path / aggregation variables (kept for backend plugins)
# ---------------------------------------------------------------------------
# add header file searching path
LIST (APPEND TENGINE_HEADER_PATH        "${CMAKE_SOURCE_DIR}/source")
LIST (APPEND TENGINE_HEADER_PATH        "${CMAKE_BINARY_DIR}/source")
LIST (APPEND TENGINE_HEADER_PATH        ${TENGINE_DEVICE_HEADER_PATH})
LIST (APPEND TENGINE_HEADER_PATH        ${TENGINE_OP_HEADER_PATH})
LIST (APPEND TENGINE_HEADER_PATH        ${TENGINE_SERIALIZER_HEADER_PATH})


# link libraries search path
LIST (APPEND TENGINE_LINK_PATH          ${TENGINE_DEVICE_LINK_PATH})


# add compiler defines
LIST (APPEND TENGINE_COMPILER_DEFINES   ${TENGINE_COMMON_DEFINES})
LIST (APPEND TENGINE_COMPILER_DEFINES   ${TENGINE_DEVICE_COMPILER_DEFINES})
LIST (APPEND TENGINE_COMPILER_DEFINES   ${TENGINE_OPERATOR_COMPILER_DEFINES})
LIST (APPEND TENGINE_COMPILER_DEFINES   ${TENGINE_SCHEDULER_COMPILER_DEFINES})
LIST (APPEND TENGINE_COMPILER_DEFINES   ${TENGINE_SERIALIZER_COMPILER_DEFINES})
LIST (APPEND TENGINE_COMPILER_DEFINES   ${TENGINE_EXECUTER_COMPILER_DEFINES})
LIST (APPEND TENGINE_COMPILER_DEFINES   ${TENGINE_EXPORTER_COMPILER_DEFINES})
LIST (APPEND TENGINE_COMPILER_DEFINES   ${TENGINE_OPTIMIZER_COMPILER_DEFINES})


# add compiler options
LIST (APPEND TENGINE_COMPILER_OPTIONS   ${TENGINE_COMMON_OPTIONS})
LIST (APPEND TENGINE_COMPILER_OPTIONS   ${TENGINE_DEVICE_COMPILER_OPTIONS})
LIST (APPEND TENGINE_COMPILER_OPTIONS   ${TENGINE_OPERATOR_COMPILER_OPTIONS})
LIST (APPEND TENGINE_COMPILER_OPTIONS   ${TENGINE_SCHEDULER_COMPILER_OPTIONS})
LIST (APPEND TENGINE_COMPILER_OPTIONS   ${TENGINE_SERIALIZER_COMPILER_OPTIONS})
LIST (APPEND TENGINE_COMPILER_OPTIONS   ${TENGINE_EXECUTER_COMPILER_OPTIONS})
LIST (APPEND TENGINE_COMPILER_OPTIONS   ${TENGINE_EXPORTER_COMPILER_OPTIONS})
LIST (APPEND TENGINE_COMPILER_OPTIONS   ${TENGINE_GRAPH_COMPILER_OPTIONS})
LIST (APPEND TENGINE_COMPILER_OPTIONS   ${TENGINE_MODULE_COMPILER_OPTIONS})
LIST (APPEND TENGINE_COMPILER_OPTIONS   ${TENGINE_OPTIMIZER_COMPILER_OPTIONS})
LIST (APPEND TENGINE_COMPILER_OPTIONS   ${TENGINE_UTILITY_COMPILER_OPTIONS})


# add linker options
LIST (APPEND TENGINE_LINKER_OPTIONS     ${TENGINE_COMMON_LINKER_OPTIONS})
LIST (APPEND TENGINE_LINKER_OPTIONS     ${TENGINE_DEVICE_LINKER_OPTIONS})
LIST (APPEND TENGINE_LINKER_OPTIONS     ${TENGINE_OPERATOR_LINKER_OPTIONS})
LIST (APPEND TENGINE_LINKER_OPTIONS     ${TENGINE_SCHEDULER_LINKER_OPTIONS})
LIST (APPEND TENGINE_LINKER_OPTIONS     ${TENGINE_SERIALIZER_LINKER_OPTIONS})
LIST (APPEND TENGINE_LINKER_OPTIONS     ${TENGINE_EXECUTER_LINKER_OPTIONS})
LIST (APPEND TENGINE_LINKER_OPTIONS     ${TENGINE_EXPORTER_LINKER_OPTIONS})
LIST (APPEND TENGINE_LINKER_OPTIONS     ${TENGINE_GRAPH_LINKER_OPTIONS})
LIST (APPEND TENGINE_LINKER_OPTIONS     ${TENGINE_MODULE_LINKER_OPTIONS})
LIST (APPEND TENGINE_LINKER_OPTIONS     ${TENGINE_OPTIMIZER_LINKER_OPTIONS})
LIST (APPEND TENGINE_LINKER_OPTIONS     ${TENGINE_UTILITY_LINKER_OPTIONS})


# add link libraries
LIST (APPEND TENGINE_LINK_LIBRARIES     ${TENGINE_COMMON_LINK_LIBRARIES})
LIST (APPEND TENGINE_LINK_LIBRARIES     ${TENGINE_DEVICE_LINK_LIBRARIES})
LIST (APPEND TENGINE_LINK_LIBRARIES     ${TENGINE_OPERATOR_LINK_LIBRARIES})
LIST (APPEND TENGINE_LINK_LIBRARIES     ${TENGINE_SCHEDULER_LINK_LIBRARIES})
LIST (APPEND TENGINE_LINK_LIBRARIES     ${TENGINE_SERIALIZER_LINK_LIBRARIES})
LIST (APPEND TENGINE_LINK_LIBRARIES     ${TENGINE_EXECUTER_LINK_LIBRARIES})
LIST (APPEND TENGINE_LINK_LIBRARIES     ${TENGINE_EXPORTER_LINK_LIBRARIES})
LIST (APPEND TENGINE_LINK_LIBRARIES     ${TENGINE_GRAPH_LINK_LIBRARIES})
LIST (APPEND TENGINE_LINK_LIBRARIES     ${TENGINE_MODULE_LINK_LIBRARIES})
LIST (APPEND TENGINE_LINK_LIBRARIES     ${TENGINE_OPTIMIZER_LINK_LIBRARIES})
LIST (APPEND TENGINE_LINK_LIBRARIES     ${TENGINE_UTILITY_LINK_LIBRARIES})


# backend plugin source files, compiled directly into the final targets until
# each backend grows its own object library
LIST (APPEND TENGINE_SOURCE_FILES       ${TENGINE_DEVICE_SOURCE})


# ---------------------------------------------------------------------------
# final targets: static and shared library
# ---------------------------------------------------------------------------
# export the module list to parent scope, cmake/summary.cmake prints it
SET (TENGINE_OBJECT_TARGETS ${TENGINE_OBJECT_TARGETS} CACHE INTERNAL "Tengine module object targets" FORCE)

ADD_LIBRARY (${TENGINE_LITE_NAME}-static STATIC)
ADD_LIBRARY (${TENGINE_LITE_NAME}        SHARED)

# add module object files to each target
FOREACH (_MODULE_TARGET ${TENGINE_OBJECT_TARGETS})
    TARGET_SOURCES (${TENGINE_LITE_NAME}-static PRIVATE $<TARGET_OBJECTS:${_MODULE_TARGET}>)
    TARGET_SOURCES (${TENGINE_LITE_NAME}        PRIVATE $<TARGET_OBJECTS:${_MODULE_TARGET}>)
ENDFOREACH()

# add backend plugin source files to each target
TARGET_SOURCES (${TENGINE_LITE_NAME}-static PRIVATE ${TENGINE_SOURCE_FILES})
TARGET_SOURCES (${TENGINE_LITE_NAME}        PRIVATE ${TENGINE_SOURCE_FILES})

# add header file search path
TARGET_INCLUDE_DIRECTORIES (${TENGINE_LITE_NAME}-static PRIVATE ${TENGINE_HEADER_PATH})
TARGET_INCLUDE_DIRECTORIES (${TENGINE_LITE_NAME}        PRIVATE ${TENGINE_HEADER_PATH})

# common build configuration, private so it never leaks to consumers
TARGET_LINK_LIBRARIES (${TENGINE_LITE_NAME}-static PRIVATE tengine_build_config)
TARGET_LINK_LIBRARIES (${TENGINE_LITE_NAME}        PRIVATE tengine_build_config)

# add link libraries search path
TARGET_LINK_DIRECTORIES (${TENGINE_LITE_NAME}-static PUBLIC ${TENGINE_LINK_PATH})
TARGET_LINK_DIRECTORIES (${TENGINE_LITE_NAME}        PUBLIC ${TENGINE_LINK_PATH})

# add defines
TARGET_COMPILE_DEFINITIONS (${TENGINE_LITE_NAME}-static PRIVATE $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:${TENGINE_COMPILER_DEFINES}>)
TARGET_COMPILE_DEFINITIONS (${TENGINE_LITE_NAME}        PRIVATE $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:${TENGINE_COMPILER_DEFINES}>)

# add options
TARGET_COMPILE_OPTIONS (${TENGINE_LITE_NAME}-static PUBLIC $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:${TENGINE_COMPILER_OPTIONS}>)
TARGET_COMPILE_OPTIONS (${TENGINE_LITE_NAME}        PUBLIC $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:${TENGINE_COMPILER_OPTIONS}>)

# add link options (target_LINKER_OPTIONS need CMake 3.13 at least)
TARGET_LINK_OPTIONS (${TENGINE_LITE_NAME}-static PUBLIC $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:${TENGINE_LINKER_OPTIONS}>)
TARGET_LINK_OPTIONS (${TENGINE_LITE_NAME}        PUBLIC $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:${TENGINE_LINKER_OPTIONS}>)

# add link libraries
TARGET_LINK_LIBRARIES (${TENGINE_LITE_NAME}-static PUBLIC  ${TENGINE_LINK_LIBRARIES})
TARGET_LINK_LIBRARIES (${TENGINE_LITE_NAME}        PUBLIC  ${TENGINE_LINK_LIBRARIES})


# ---------------------------------------------------------------------------
# toolchain and OpenMP flags for module object libraries
# ---------------------------------------------------------------------------
# openmp detection first, object libraries need the compile flags as well,
# otherwise #pragma omp would be silently ignored
UNSET (_has_openmp)
IF (NOT (TENGINE_FORCE_SKIP_OPENMP) AND TENGINE_OPENMP)
    INCLUDE (${PROJECT_SOURCE_DIR}/cmake/libraries/openmp.cmake)
    TENGINE_CHECK_LIB_OPENMP (_has_openmp)
ENDIF()

FOREACH (_MODULE_TARGET ${TENGINE_OBJECT_TARGETS})
    # objects are linked into both the static and the shared library,
    # position independent code is required by the shared one
    SET_TARGET_PROPERTIES (${_MODULE_TARGET} PROPERTIES POSITION_INDEPENDENT_CODE ON)

    TENGINE_APPLY_TOOLCHAIN_FLAGS (${_MODULE_TARGET})
    IF (_has_openmp)
        TENGINE_COMPILE_LIB_OPENMP (${_MODULE_TARGET})
    ENDIF()

    # strip unused sections, helps reducing the size of the static library
    IF (TENGINE_COMPILER_GCC OR TENGINE_COMPILER_CLANG)
        TARGET_COMPILE_OPTIONS (${_MODULE_TARGET} PRIVATE $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:-fdata-sections>)
        TARGET_COMPILE_OPTIONS (${_MODULE_TARGET} PRIVATE $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:-ffunction-sections>)

        # hide internal symbols in the shared library, the public C API stays
        # visible through the DLLEXPORT attribute in c_api.h
        IF (NOT TENGINE_ENABLE_ALL_SYMBOL)
            TARGET_COMPILE_OPTIONS (${_MODULE_TARGET} PRIVATE $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:-fvisibility=hidden>)
        ENDIF()
    ENDIF()
ENDFOREACH()


# default support openmp
IF (_has_openmp)
    TENGINE_USE_LIB_OPENMP (${TENGINE_LITE_NAME}-static ON)
    TENGINE_USE_LIB_OPENMP (${TENGINE_LITE_NAME}        OFF)
    IF(MSVC)
        SET_SOURCE_FILES_PROPERTIES(${TENGINE_SOURCE_FILES} PROPERTIES LANGUAGE CXX)
        TARGET_COMPILE_OPTIONS(${TENGINE_LITE_NAME}-static PRIVATE /std:c++latest)
        TARGET_COMPILE_OPTIONS(${TENGINE_LITE_NAME}        PRIVATE /std:c++latest)
    ENDIF()

    SET (TENGINE_ENABLED_OPENMP ${OpenMP_C_VERSION} CACHE INTERNAL "" FORCE)
ENDIF()
UNSET (_has_openmp)

# pthread
IF (NOT OHOS)
	INCLUDE (${PROJECT_SOURCE_DIR}/cmake/libraries/pthread.cmake)
	TENGINE_CHECK_LIB_PTHREAD (_has_pthread)
	IF (_has_pthread)
		TENGINE_USE_LIB_PTHREAD (${TENGINE_LITE_NAME}-static ON)
		TENGINE_USE_LIB_PTHREAD (${TENGINE_LITE_NAME}        OFF)
	ENDIF()
	UNSET (_has_pthread)
ENDIF()


#
IF (TENGINE_COMPILER_GCC OR TENGINE_COMPILER_CLANG)
    # linking time optimization
    IF ((NOT ANDROID) AND (NOT OHOS))
        #TARGET_COMPILE_OPTIONS (${TENGINE_LITE_NAME} PRIVATE $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:-flto>)
    ENDIF()

    IF (NOT TENGINE_ENABLE_ALL_SYMBOL)
        TARGET_COMPILE_OPTIONS (${TENGINE_LITE_NAME} PRIVATE $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:-fvisibility=hidden>)
    ENDIF()

    # it seems will lead to some error
    #ADD_COMPILE_OPTIONS ($<$<COMPILE_LANGUAGE:CXX>:-Wl,-rpath,'$ORIGIN:/lib>)

    #SET_TARGET_PROPERTIES(${TENGINE_LITE_NAME}-static PROPERTIES LINK_FLAGS "-Wl,--exclude-libs,ALL")
ENDIF()


# apply external toolchain flags to the final targets
TENGINE_APPLY_TOOLCHAIN_FLAGS (${TENGINE_LITE_NAME}-static)
TENGINE_APPLY_TOOLCHAIN_FLAGS (${TENGINE_LITE_NAME})


# cuda above option
IF (TENGINE_ENABLE_CUDA)
    IF (TENGINE_ENV_HAS_CXX14)
        SET_PROPERTY(TARGET ${TENGINE_LITE_NAME}-static PROPERTY CUDA_STANDARD 14)
        SET_PROPERTY(TARGET ${TENGINE_LITE_NAME}        PROPERTY CUDA_STANDARD 14)
    ELSE()
        SET_PROPERTY(TARGET ${TENGINE_LITE_NAME}-static PROPERTY CUDA_STANDARD 11)
        SET_PROPERTY(TARGET ${TENGINE_LITE_NAME}        PROPERTY CUDA_STANDARD 11)
    ENDIF()
    SET_PROPERTY(TARGET ${TENGINE_LITE_NAME}-static PROPERTY CUDA_STANDARD_REQUIRED ON)
    SET_PROPERTY(TARGET ${TENGINE_LITE_NAME}        PROPERTY CUDA_STANDARD_REQUIRED ON)

    TARGET_COMPILE_OPTIONS (${TENGINE_LITE_NAME}-static PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-Werror=cross-execution-space-call>)
    TARGET_COMPILE_OPTIONS (${TENGINE_LITE_NAME}        PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-Werror=cross-execution-space-call>)

    IF (${CMAKE_MINOR_VERSION} GREATER 17)
        IF (TENGINE_TARGET_PROCESSOR MATCHES "ARM")
            SET_PROPERTY(TARGET ${TENGINE_LITE_NAME}-static PROPERTY CUDA_ARCHITECTURES 53 62 72)
            SET_PROPERTY(TARGET ${TENGINE_LITE_NAME}        PROPERTY CUDA_ARCHITECTURES 53 62 72)
        ELSE()
            SET_PROPERTY(TARGET ${TENGINE_LITE_NAME}-static PROPERTY CUDA_ARCHITECTURES 50 52 61 70 75)
            SET_PROPERTY(TARGET ${TENGINE_LITE_NAME}        PROPERTY CUDA_ARCHITECTURES 50 52 61 70 75)
        ENDIF()
    ENDIF()
ENDIF()

# deal with depends
FOREACH(_var ${TENGINE_DEVICE_DEPENDS_FORWARD})
    ADD_DEPENDENCIES(${TENGINE_LITE_NAME}-static ${_var})
    ADD_DEPENDENCIES(${TENGINE_LITE_NAME}        ${_var})
ENDFOREACH()

# debug macro information, applied on module object libraries so both the
# static and shared library behave the same (all switches default to OFF)
SET (_TENGINE_DEBUG_DEFINES)
IF (TENGINE_DEBUG_MEM_STAT)
    LIST (APPEND _TENGINE_DEBUG_DEFINES CONFIG_MEM_STAT)
ENDIF()
IF (TENGINE_DEBUG_DATA)
    LIST (APPEND _TENGINE_DEBUG_DEFINES DEBUG_DATA)
ENDIF()
IF (TENGINE_DEBUG_TIME)
    LIST (APPEND _TENGINE_DEBUG_DEFINES DEBUG_TIME)
ENDIF()
IF (TENGINE_LITE_VERSION)
    LIST (APPEND _TENGINE_DEBUG_DEFINES TENGINE_LITE_VERSION=${TENGINE_LITE_VERSION})
ENDIF()
IF (TENGINE_VERSION_POSTFIX)
    LIST (APPEND _TENGINE_DEBUG_DEFINES TENGINE_VERSION_POSTFIX=${TENGINE_VERSION_POSTFIX})
ENDIF()

IF (_TENGINE_DEBUG_DEFINES)
    FOREACH (_MODULE_TARGET ${TENGINE_OBJECT_TARGETS})
        TARGET_COMPILE_DEFINITIONS(${_MODULE_TARGET} PRIVATE ${_TENGINE_DEBUG_DEFINES})
    ENDFOREACH()

    # backend plugin sources are compiled inside the final targets
    TARGET_COMPILE_DEFINITIONS(${TENGINE_LITE_NAME}-static PRIVATE $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:${_TENGINE_DEBUG_DEFINES}>)
    TARGET_COMPILE_DEFINITIONS(${TENGINE_LITE_NAME}        PRIVATE $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:${_TENGINE_DEBUG_DEFINES}>)
ENDIF()
UNSET (_TENGINE_DEBUG_DEFINES)

SET_PROPERTY(TARGET ${CMAKE_PROJECT_NAME}-static PROPERTY FOLDER "libtengine-lite")
SET_PROPERTY(TARGET ${CMAKE_PROJECT_NAME} PROPERTY FOLDER "libtengine-lite")

# install tengine libraries
INSTALL (TARGETS ${CMAKE_PROJECT_NAME}-static DESTINATION lib)
INSTALL (TARGETS ${CMAKE_PROJECT_NAME}        DESTINATION lib)

IF (TENGINE_STANDALONE_HCL)
    INSTALL (TARGETS ${TENGINE_STANDALONE_HCL_LIB_NAME} DESTINATION lib)
ENDIF()

# install header files
INSTALL (FILES ${CMAKE_SOURCE_DIR}/source/api/c_api.h DESTINATION include/tengine)
INSTALL (FILES ${CMAKE_CURRENT_BINARY_DIR}/defines.h  DESTINATION include/tengine)

IF (TENGINE_BUILD_CPP_API)
    INSTALL (FILES ${CMAKE_SOURCE_DIR}/source/api/cpp_api.h DESTINATION include/tengine)
ENDIF()
