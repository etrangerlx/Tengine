# this function could get target system bit width in bytes

macro(TENGINE_CHECK_SYS_WIDTH _bit_width)
    math(EXPR ${_bit_width} "8 * ${CMAKE_SIZEOF_VOID_P}" OUTPUT_FORMAT DECIMAL)
endmacro()
