# 0. clear var
unset (_DEV_OCL_HEADER_PATH)
unset (_OCL_BASE_SOURCE)
unset (_OCL_OPS_SOURCE)
unset (_DEV_OCL_DEVICE_SOURCE)
unset (_DEV_OCL_COMPILER_DEFINES)
unset (_DEV_OCL_COMPILER_OPTIONS)
unset (_DEV_OCL_LINKER_OPTIONS)
unset (_DEV_OCL_LINK_LIBRARIES)


# add link options
option(TENGINE_OPENCL_PROFILE_TIME "enable opencl profile time" OFF)
if (TENGINE_OPENCL_PROFILE_TIME)
    list(APPEND _DEV_OCL_COMPILER_DEFINES OPENCL_PROFILE_TIME)
endif ()
option(TENGINE_OPENCL_MODEL_CACHE "enable opencl cache" ON)
option(TENGINE_OPENCL_DEBUG_DATA "enable node tensor debug data" OFF)
if (TENGINE_OPENCL_DEBUG_DATA)
    list(APPEND _DEV_OCL_COMPILER_DEFINES OPENCL_DEBUG_DATA)
endif()


#  set source root path
set(_OCL_ROOT ${CMAKE_SOURCE_DIR}/source/device/opencl)


# 2.  add header file path
list (APPEND _DEV_OCL_HEADER_PATH           ${_OCL_ROOT})
list (APPEND _DEV_OCL_HEADER_PATH           ${_OCL_ROOT}/include)

# add source files
aux_source_directory("${_OCL_ROOT}" _OCL_BASE_SOURCE)
aux_source_directory("${_OCL_ROOT}/oppack4" _OCL_OPS_SOURCE_PACK4)
list(APPEND _DEV_OCL_DEVICE_SOURCE "${_OCL_ROOT}/cl4/ocl_program_hex.cc")
list(APPEND _DEV_OCL_DEVICE_SOURCE ${_OCL_BASE_SOURCE})
list(APPEND _DEV_OCL_DEVICE_SOURCE ${_OCL_OPS_SOURCE})
list(APPEND _DEV_OCL_DEVICE_SOURCE ${_OCL_OPS_SOURCE_PACK4})

if (TENGINE_OPENCL_MODEL_CACHE)
    aux_source_directory("${_OCL_ROOT}/cache" _OCL_CACHE)
    list(APPEND _DEV_OCL_DEVICE_SOURCE ${_OCL_CACHE})
endif ()


# add build options for cpu device
# is a gcc or clang like compiler
if (TENGINE_COMPILER_GCC OR TENGINE_COMPILER_CLANG)
    if (TENGINE_COMPILER_GCC AND (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL "6.1"))
        list(APPEND _DEV_OCL_COMPILER_OPTIONS -Wno-ignored-attributes)
    endif ()
endif ()


find_package(OpenCL)
if (NOT OpenCL_FOUND)
    message(WARNING "please set opencl library path manual")
endif ()
list(APPEND _DEV_OCL_LINK_LIBRARIES ${OpenCL_LIBRARY})


# 8. set all to cmake cache
set (TENGINE_OCL_HEADER_PATH       ${_DEV_OCL_HEADER_PATH}        CACHE INTERNAL  "Tengine OpenCL device header files searching path"   FORCE)
set (TENGINE_OCL_LINK_PATH         ${_DEV_OCL_LINK_PATH}          CACHE INTERNAL  "Tengine OpenCL device link libraries searching path" FORCE)
set (TENGINE_OCL_DEVICE_SOURCE     ${_DEV_OCL_DEVICE_SOURCE}      CACHE INTERNAL  "Tengine OpenCL device main source files"             FORCE)
set (TENGINE_OCL_COMPILER_DEFINES  ${_DEV_OCL_COMPILER_DEFINES}   CACHE INTERNAL  "Tengine OpenCL about compiler defines"               FORCE)
set (TENGINE_OCL_COMPILER_OPTIONS  ${_DEV_OCL_COMPILER_OPTIONS}   CACHE INTERNAL  "Tengine OpenCL about compiler options"               FORCE)
set (TENGINE_OCL_LINKER_OPTIONS    ${_DEV_OCL_LINKER_OPTIONS}     CACHE INTERNAL  "Tengine OpenCL about linker options"                 FORCE)
set (TENGINE_OCL_LINK_LIBRARIES    ${_DEV_OCL_LINK_LIBRARIES}     CACHE INTERNAL  "Tengine OpenCL about link libraries"                 FORCE)


# 9. install device option
install (FILES ${_OCL_ROOT}/ocl_define.h DESTINATION include/tengine RENAME ocl_device.h)
