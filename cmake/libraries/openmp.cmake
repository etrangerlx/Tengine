# find & use OpenMP
#   CMake FindOpenMP module: see https://cmake.org/cmake/help/latest/module/FindOpenMP.html
#     version    OPENMP_FOUND    OpenMP_<lang>_FOUND    OpenMP_<lang>_FLAGS    OpenMP::OpenMP_<lang>    OpenMP_<lang>_VERSION    OpenMP_<lang>_SPEC_DATE    OpenMP_<lang>_INCLUDE_DIRS    OpenMP_<lang>_LIBRARIES
#       3.0           V                  NA                    C/C++                    NA                       NA                        NA                          NA                           NA
#       3.1           V                  NA                C/C++/Fortran                NA                       NA                        NA                          NA                           NA
#       3.7           V                  NA                C/C++/Fortran                NA                       NA                   C/C++/Fortran                    NA                           NA
#       3.9      OpenMP_FOUND       C/C++/Fortran          C/C++/Fortran           C/C++/Fortran            C/C++/Fortran             C/C++/Fortran                    NA                      C/C++/Fortran
#       3.16     OpenMP_FOUND       C/C++/Fortran          C/C++/Fortran           C/C++/Fortran            C/C++/Fortran             C/C++/Fortran               C/C++/Fortran                C/C++/Fortran
#       3.19
#
#   OpenMP Compilers: see https://www.openmp.org//resources/openmp-compilers-tools
#     

macro(TENGINE_CHECK_LIB_OPENMP _has_flag)
    # use find_package to find openmp
    find_package(OpenMP)

    # OPENMP_FOUND: <  3.9
    # OpenMP_FOUND: >= 3.9
    if(OPENMP_FOUND OR OpenMP_FOUND OR OpenMP_C_FOUND OR OpenMP_CXX_FOUND)
        set(${_has_flag} ON)
    endif()
endmacro()


function(TENGINE_COMPILE_LIB_OPENMP _target)
    if (NOT(OpenMP_C_FLAGS OR OpenMP_CXX_FLAGS))
        tengine_check_lib_openmp(_has_openmp)
    else()
        target_compile_options(${_target} PRIVATE $<$<COMPILE_LANGUAGE:C>:${OpenMP_C_FLAGS}> $<$<COMPILE_LANGUAGE:CXX>:${OpenMP_CXX_FLAGS}>)
    endif()

    unset(_has_openmp)
endfunction()


function(TENGINE_LINK_LIB_OPENMP _target _is_static)
    if (NOT(OpenMP_C_FOUND OR OpenMP_C_FOUND) AND NOT(OpenMP_C_FLAGS OR OpenMP_CXX_FLAGS))
        tengine_check_lib_openmp(_has_openmp)
    else()
        if(((TARGET OpenMP::OpenMP_C) OR (TARGET OpenMP::OpenMP_CXX)) AND NOT ANDROID_NDK)
            # LINK_LANGUAGE keyword is added to generator expressions in 3.18
            if((${CMAKE_MAJOR_VERSION} VERSION_EQUAL 3) AND (${CMAKE_MINOR_VERSION} VERSION_EQUAL 18 OR ${CMAKE_MINOR_VERSION} VERSION_GREATER 18))
                if (${_is_static})
                    target_link_libraries(${_target} PUBLIC  $<$<LINK_LANGUAGE:C>:OpenMP::OpenMP_C>)
                    target_link_libraries(${_target} PUBLIC  $<$<LINK_LANGUAGE:CXX>:OpenMP::OpenMP_CXX>)
                else()
                    target_link_libraries(${_target} PRIVATE $<$<LINK_LANGUAGE:C>:OpenMP::OpenMP_C>)
                    target_link_libraries(${_target} PRIVATE $<$<LINK_LANGUAGE:CXX>:OpenMP::OpenMP_CXX>)
                endif()
            else()
                if (${_is_static})
                    target_link_libraries(${_target} PUBLIC  ${OpenMP_C_FLAGS})
                else()
                    target_link_libraries(${_target} PRIVATE ${OpenMP_C_FLAGS})
                endif()
            endif()
        elseif(ANDROID_NDK_MAJOR AND (ANDROID_NDK_MAJOR GREATER 20))
            if (${_is_static})
                target_link_libraries(${_target} PUBLIC  "${OpenMP_C_FLAGS} -static-openmp")
            else()
                target_link_libraries(${_target} PRIVATE "${OpenMP_C_FLAGS} -static-openmp")
            endif()
        else()
            if (${_is_static})
                target_link_libraries(${_target} PUBLIC  ${OpenMP_C_FLAGS})
            else()
                target_link_libraries(${_target} PRIVATE ${OpenMP_C_FLAGS})
            endif()
        endif()
    endif()

    unset(_has_openmp)
endfunction()


function(TENGINE_USE_LIB_OPENMP _target _is_static)
    tengine_compile_lib_openmp(${_target})
    tengine_link_lib_openmp(${_target} ${_is_static})
endfunction()
