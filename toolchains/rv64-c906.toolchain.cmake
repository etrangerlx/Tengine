# set cross-compiled system type, it's better not use the type which cmake cannot recognized.
set (CMAKE_SYSTEM_NAME Linux)
set (CMAKE_SYSTEM_PROCESSOR riscv64)

# riscv64-unknown-linux-gnu DO NOT need to be installed, so make sure riscv64-unknown-linux-gnu-gcc and riscv64-unknown-linux-gnu-g++ can be found in $PATH:
set (CMAKE_C_COMPILER   "riscv64-unknown-linux-gnu-gcc")
set (CMAKE_CXX_COMPILER "riscv64-unknown-linux-gnu-g++")

# set searching rules for cross-compiler
set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# other needed options
set (TENGINE_TOOLCHAIN_ASM_FLAG -march=rv64gcvxthead -mabi=lp64d -mtune=c906 -mfp16 -lc)
#SET (TENGINE_TOOLCHAIN_FLAG -march=rv64imafdcvxtheadc -mabi=lp64dv -mtune=c906 -mfp16)
#SET (TENGINE_TOOLCHAIN_FLAG -march=rv64imafdcvxtheadc -mabi=lp64dv -mtune=c910 -mfp16)

# skip OpenMP check as default, for early RISC-V only have single CPU core
set (TENGINE_FORCE_SKIP_OPENMP ON)

# Note:
#   Early toolchain was not usable enough, so users should adjust TENGINE_TOOLCHAIN_FLAG for a compilation pass
