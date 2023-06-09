# cmake version main changes:
#
# 3.11: generator expression for Xcode and Visual Studio added
#       INTERFACE support added
# 3.12: OpenMP support on Mac OS added
#       CUDA as a language added
#       -j,--parallel in --build added
#       new FindPython module added
#       C++20 support added
# 3.13: symbolic links on Windows added
#       target_link_options and add_link_options added
#       target_link_directories added
#       STATIC_LIBRARY_OPTIONS added
#       CUDA 10 (Turing) detection added
# 3.14: several utilities added
#       BUILD_RPATH_USE_ORIGIN added
# 3.15: some command line improve
# 3.16: Objective C and Objective C++ languages support added
#       target_precompile_headers support added
#       RPath control added
# 3.17: FindCUDAToolkit added
#       CUDA_RUNTIME_LIBRARY static linking mode added
#       FindPython with Conda added


# Check CMake version
CMAKE_MINIMUM_REQUIRED (VERSION 3.10 FATAL_ERROR)

# Disable inplace builds to prevent source tree corruption.
IF (" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
    MESSAGE (FATAL_ERROR "FATAL: Building inplace are not allowed. You should create a separate directory for Building.")
ENDIF ()

# set cmake_install_prefix path
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation Directory")
endif()
message(STATUS "CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}")
set(CMAKE_CXX_COMPILER_WORKS 1)
# Enable the languages which in use
ENABLE_LANGUAGE (C CXX)

if(CMAKE_TOOLCHAIN_FILE)
    set(LIBRARY_OUTPUT_PATH_ROOT ${CMAKE_BINARY_DIR} CACHE PATH "root for library output, set this to change where android libs are compiled to")
    # get absolute path, but get_filename_component ABSOLUTE only refer with source dir, so find_file here :(
    get_filename_component(CMAKE_TOOLCHAIN_FILE_NAME ${CMAKE_TOOLCHAIN_FILE} NAME)
    find_file(CMAKE_TOOLCHAIN_FILE ${CMAKE_TOOLCHAIN_FILE_NAME} PATHS ${CMAKE_SOURCE_DIR} NO_DEFAULT_PATH)
    message(STATUS "CMAKE_TOOLCHAIN_FILE = ${CMAKE_TOOLCHAIN_FILE}")
endif()


if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE release CACHE STRING "Choose the type of build" FORCE)
endif()

# Do check list
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/check.cmake")

project(tengine-lite)

option(TENGINE_OPENMP "openmp support" ON)
option(TENGINE_BUILD_BENCHMARK "build examples" ON)
option(TENGINE_BUILD_EXAMPLES "build benchmark" ON)
option(TENGINE_BUILD_TESTS "build tests" OFF)
option(TENGINE_DEBUG_DATA "extract data for every layer" OFF)
option(TENGINE_DEBUG_TIME "print time information for every layer" OFF)
option(TENGINE_DEBUG_MEM_STAT "print memory status for library" OFF)


# add_definitions(-DCONFIG_DISABLE_PARAM_ACCESS)
# add_definitions(-DCONFIG_INTERN_ALLOCATOR)
add_definitions(-DCONFIG_TENGINE_SERIALIZER)

# Debug macro information
if (TENGINE_BUILD_BENCHMARK)
    add_definitions(-DCONFIG_MEM_STAT)
endif()
if (TENGINE_DEBUG_DATA)
    add_definitions(-DDEBUG_DATA)
endif()
if (TENGINE_DEBUG_TIME)
    add_definitions(-DDEBUG_TIME)
endif()

# add_definitions(-g)
# add_definitions(-O0)
add_definitions(-fPIC)
add_definitions(-O2)

if (${TENGINE_TARGET_PROCESSOR} MATCHES "ARM")
    if (TENGINE_TARGET_PROCESSOR_32Bit AND NOT ANDROID)
        add_definitions(-march=armv7-a)
        add_definitions(-mfpu=neon)
        add_definitions(-mfp16-format=ieee)
        add_definitions(-mfpu=neon-fp16)
    endif()

    if (TENGINE_TOOLCHIN_FLAG)
        add_definitions ("${TENGINE_TOOLCHIN_FLAG}")
    endif()
endif()

enable_language(ASM)

# local include files
include_directories(include/)
include_directories(src/op/)

# add OpenMP support

if (NOT TENGINE_FORCE_SKIP_OPENMP)
    find_package(OpenMP)
    if(NOT TARGET OpenMP::OpenMP_CXX AND (OpenMP_CXX_FOUND OR OPENMP_FOUND))
    list (APPEND TENGINE_CXX_PRIVATE_COMPILE_OPTIONS ${OpenMP_CXX_FLAGS})
    #message("Old mode, gcc build with ${OpenMP_CXX_FLAGS}")

        set (TENGINE_WITH_OPENMP ON)
    endif()
endif()

if(TENGINE_WITH_OPENMP AND (OpenMP_CXX_FOUND OR OPENMP_FOUND))
    if(TENGINE_VERBOSE)
        message (STATUS "TENGINE: OpenMP support enabled.")
    endif()

    if(OpenMP_CXX_FOUND)
        list (APPEND TENGINE_CXX_LINKING_LIBS OpenMP::OpenMP_CXX)
        #message("New mode, gcc build with linking OpenMP::OpenMP_CXX")
    else()
        list (APPEND TENGINE_CXX_LINKING_LIBS "${OpenMP_CXX_FLAGS}")
        #message("Old mode, gcc build with linking ${OpenMP_CXX_FLAGS}")
    endif()
endif()

# Report summary
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/summary.cmake")

# enable cmake ctest module
enable_testing()

# src files
add_subdirectory(src)
if (TENGINE_BUILD_BENCHMARK)
    add_subdirectory(benchmark)
endif()
if (TENGINE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
if (TENGINE_BUILD_TESTS)
    # add_subdirectory(test) todo
endif()
