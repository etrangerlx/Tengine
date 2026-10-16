# set cross-compiled system type, it's better not use the type which cmake cannot recognized.
set (CMAKE_SYSTEM_NAME Linux)
set (CMAKE_SYSTEM_PROCESSOR mips64r2)

# when mips-linux-gnu-gcc was installed, toolchain was available as below:
set (CMAKE_C_COMPILER   "mips-linux-gnu-gcc")
set (CMAKE_CXX_COMPILER "mips-linux-gnu-g++")

# set searching rules for cross-compiler
set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# other needed options
set (TENGINE_TOOLCHAIN_FLAG -march=mips64r2 -mabi=64 -mmsa -mhard-float -mfp64)

# do not skip OpenMP check as default
set (TENGINE_FORCE_SKIP_OPENMP OFF)
