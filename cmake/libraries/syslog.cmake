# check <syslog.h>

function(TENGINE_CHECK_INC_SYSLOG _has_flag)
    if(CMAKE_C_COMPILER_LOADED)
        include(CheckIncludeFile)
        check_include_file("syslog.h" ${_has_flag})
    else()
        include(CheckIncludeFileCXX)
        check_include_file_cxx("syslog.h" ${_has_flag})
    endif()
endfunction()
