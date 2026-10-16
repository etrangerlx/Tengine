# this function will check if gcc is hard float point

function(TENGINE_CHECK_ARM32_HARD_FP _flag)
    set (_compile_flag "-march=armv7-a -mfpu=neon -mfloat-abi=hard")
    check_c_compiler_flag (${_compile_flag} ${_flag})
endfunction()
