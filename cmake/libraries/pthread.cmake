# find & use pthread

function(TENGINE_CHECK_LIB_PTHREAD _has_flag)
    if(CMAKE_C_COMPILER_LOADED)
        include(CheckIncludeFile)
        check_include_file("pthread.h" ${_has_flag})
    else()
        include(CheckIncludeFileCXX)
        check_include_file_cxx("pthread.h" ${_has_flag})
    endif()
endfunction()


function(TENGINE_COMPILE_LIB_PTHREAD _target)
    # common compile option settings
    if (TENGINE_ENABLE_CUDA OR TENGINE_ENABLE_TENSORRT)
        target_compile_options(${_target} PRIVATE $<$<COMPILE_LANG_AND_ID:CUDA,NVIDIA>:SHELL:-Xcompiler -pthread> $<$<NOT:$<COMPILE_LANG_AND_ID:CUDA,NVIDIA>>:-pthread>)
    endif()

    # accoding to EMSCRIPTEN doc, compile & link option "-pthread" was needed.
    # if settings worked, macro "__EMSCRIPTEN_PTHREADS__" will be defined. 
    # see https://emscripten.org/docs/porting/pthreads.html
    if(EMSCRIPTEN)
        # TARGET_LINK_OPTIONS was added after 3.13
        # see https://cmake.org/cmake/help/latest/command/target_link_options.html
        cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

        # LINK_LANGUAGE keyword is added to generator expressions in 3.18
        if((${CMAKE_MAJOR_VERSION} VERSION_EQUAL 3) AND (${CMAKE_MINOR_VERSION} VERSION_EQUAL 18 OR ${CMAKE_MINOR_VERSION} VERSION_GREATER 18))
            target_link_options(${_target} PRIVATE $<$<LINK_LANGUAGE:C>:-pthread> $<$<LINK_LANGUAGE:CXX>:-pthread>)
        else()
            target_link_options(${_target} PRIVATE -pthread)
        endif()
    endif()
endfunction()


function(TENGINE_LINK_LIB_PTHREAD _target _is_static)
    if(ANDROID)
        # is libandroid.so enough for using pthread?
        if (_is_static)
            target_link_libraries(${_target} PUBLIC  android)
        else()
            target_link_libraries(${_target} PRIVATE android)
        endif()
    elseif(UNIX AND NOT APPLE AND NOT MSVC)
        if (_is_static)
            target_link_libraries(${_target} PUBLIC  pthread)
        else()
            target_link_libraries(${_target} PRIVATE pthread)
        endif()
    elseif(EMSCRIPTEN)
        # nothing need to do when building for emscripten
    else()
        if(TENGINE_VERBOSE)
            message (AUTHOR_WARNING "FULCRUM(VERBOSE): CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}, a condition check should be added(TENGINE_LINK_PTHREAD).")
        endif()
        if (_is_static)
            target_link_libraries(${_target} PUBLIC  pthread)
        else()
            target_link_libraries(${_target} PRIVATE pthread)
        endif()
    endif()
endfunction()


function(TENGINE_USE_LIB_PTHREAD _target _is_static)
    tengine_compile_lib_pthread(${_target})
    tengine_link_lib_pthread(${_target} ${_is_static})
endfunction()
