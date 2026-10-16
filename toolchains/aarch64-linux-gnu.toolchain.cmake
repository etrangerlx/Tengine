# set cross-compiled system type, it's better not use the type which cmake cannot recognized.
set (CMAKE_SYSTEM_NAME Linux)
set (CMAKE_SYSTEM_PROCESSOR aarch64)

# aarch64-linux-gnu-gcc DO NOT need to be installed, so make sure aarch64-linux-gnu-gcc and aarch64-linux-gnu-g++ can be found in $PATH:
set (CMAKE_C_COMPILER   "aarch64-linux-gnu-gcc")
set (CMAKE_CXX_COMPILER "aarch64-linux-gnu-g++")

# set searching rules for cross-compiler
set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# do not skip OpenMP check as default
set (TENGINE_FORCE_SKIP_OPENMP OFF)
