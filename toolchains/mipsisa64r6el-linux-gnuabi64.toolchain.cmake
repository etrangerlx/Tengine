# set cross-compiled system type, it's better not use the type which cmake cannot recognized.
set (CMAKE_SYSTEM_NAME Linux)
set (CMAKE_SYSTEM_PROCESSOR mips64r2)

# when mips-linux-gnu-gcc was installed, toolchain was available as below:
set (CMAKE_C_COMPILER   "mipsisa64r6el-linux-gnuabi64-gcc")
set (CMAKE_CXX_COMPILER "mipsisa64r6el-linux-gnuabi64-g++")

# set searching rules for cross-compiler
set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# other needed options
set (TENGINE_TOOLCHAIN_FLAG -march=mips64r6 -mmsa -mhard-float -mfp64 -mnan=2008)

# do not skip OpenMP check as default
set (TENGINE_FORCE_SKIP_OPENMP OFF)
