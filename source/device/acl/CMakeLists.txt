# 0. clear var
unset (_DEV_ACL_HEADER_PATH)
unset (_ACL_BASE_SOURCE)
unset (_ACL_OPS_SOURCE)
unset (_DEV_ACL_DEVICE_SOURCE)
unset (_DEV_ACL_COMPILER_DEFINES)
unset (_DEV_ACL_COMPILER_OPTIONS)
unset (_DEV_ACL_LINKER_OPTIONS)
unset (_DEV_ACL_LINK_LIBRARIES)


# 1.  set source root path
set(_ACL_ROOT ${CMAKE_SOURCE_DIR}/source/device/acl)


# 2.  add header file path
list (APPEND _DEV_ACL_HEADER_PATH      ${_ACL_ROOT})
list (APPEND _DEV_ACL_HEADER_PATH      ${CMAKE_SOURCE_DIR}/3rdparty/acl/include)


# 3.  add linking lib searching path
list (APPEND _DEV_ACL_LINK_PATH        ${CMAKE_SOURCE_DIR}/3rdparty/acl/lib)


# 4.  add source files
aux_source_directory("${_ACL_ROOT}"    _ACL_BASE_SOURCE)
aux_source_directory("${_ACL_ROOT}/op" _ACL_OPS_SOURCE)
list (APPEND _DEV_ACL_DEVICE_SOURCE    ${_ACL_BASE_SOURCE})
list (APPEND _DEV_ACL_DEVICE_SOURCE    ${_ACL_OPS_SOURCE})


# 5.  add build options for cpu device
# 5.1 is a gcc or clang like compiler
if (TENGINE_COMPILER_GCC OR TENGINE_COMPILER_CLANG)
    if (TENGINE_COMPILER_GCC AND (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL "6.1"))
        list (APPEND _DEV_ACL_COMPILER_OPTIONS -Wno-ignored-attributes)
    endif()
endif()


# 5.2 is Microsoft Visual C++
if (TENGINE_COMPILER_MSVC)
endif()


# 6.  add link options


# 7.  add link libs
list (APPEND _DEV_ACL_LINK_LIBRARIES   arm_compute)
list (APPEND _DEV_ACL_LINK_LIBRARIES   arm_compute_core)


# 8. set all to cmake cache
set (TENGINE_ACL_HEADER_PATH       ${_DEV_ACL_HEADER_PATH}        CACHE INTERNAL  "Tengine Arm Compute Library device header files searching path"   FORCE)
set (TENGINE_ACL_LINK_PATH         ${_DEV_ACL_LINK_PATH}          CACHE INTERNAL  "Tengine Arm Compute Library device link libraries searching path" FORCE)
set (TENGINE_ACL_DEVICE_SOURCE     ${_DEV_ACL_DEVICE_SOURCE}      CACHE INTERNAL  "Tengine Arm Compute Library device main source files"             FORCE)
set (TENGINE_ACL_COMPILER_DEFINES  ${_DEV_ACL_COMPILER_DEFINES}   CACHE INTERNAL  "Tengine Arm Compute Library about compiler defines"               FORCE)
set (TENGINE_ACL_COMPILER_OPTIONS  ${_DEV_ACL_COMPILER_OPTIONS}   CACHE INTERNAL  "Tengine Arm Compute Library about compiler options"               FORCE)
set (TENGINE_ACL_LINKER_OPTIONS    ${_DEV_ACL_LINKER_OPTIONS}     CACHE INTERNAL  "Tengine Arm Compute Library about linker options"                 FORCE)
set (TENGINE_ACL_LINK_LIBRARIES    ${_DEV_ACL_LINK_LIBRARIES}     CACHE INTERNAL  "Tengine Arm Compute Library about link libraries"                 FORCE)


# 9. install device option
install (FILES ${_ACL_ROOT}/acl_define.h DESTINATION include/tengine RENAME acl_device.h)
