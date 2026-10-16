# add source file to list, and add to special visual folder
function (TENGINE_SOURCE_GROUP _FOLDER_NAME)
	if (MSVC)
		string (REGEX REPLACE "/" "\\\\" _TARGET_FOLDER "${_FOLDER_NAME}")
    endif()

    if (ARGN)
        foreach (_FILE ${ARGN})
            source_group ("${_TARGET_FOLDER}" FILES "${_FILE}")
        endforeach()
    endif()
endfunction()

# apply external toolchain flags to a normal (non-interface) target,
# each list is bound to its source language
function (TENGINE_APPLY_TOOLCHAIN_FLAGS _TARGET)
    foreach (_OPT ${TENGINE_TOOLCHAIN_FLAG})
        target_compile_options (${_TARGET} PRIVATE $<$<NOT:$<COMPILE_LANGUAGE:CUDA>>:${_OPT}>)
    endforeach()

    foreach (_OPT ${TENGINE_TOOLCHAIN_ASM_FLAG})
        target_compile_options (${_TARGET} PRIVATE $<$<COMPILE_LANGUAGE:ASM>:${_OPT}>)
    endforeach()

    foreach (_OPT ${TENGINE_TOOLCHAIN_C_FLAG})
        target_compile_options (${_TARGET} PRIVATE $<$<COMPILE_LANGUAGE:C>:${_OPT}>)
    endforeach()

    foreach (_OPT ${TENGINE_TOOLCHAIN_CXX_FLAG})
        target_compile_options (${_TARGET} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${_OPT}>)
    endforeach()
endfunction()
