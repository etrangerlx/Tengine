# aarch64-himix100: Hi3556AV100

# set cross-compiled system type, it's better not use the type which cmake cannot recognized.
set (CMAKE_SYSTEM_NAME Linux)
set (CMAKE_SYSTEM_PROCESSOR aarch64)

# when HiSilicon SDK was installed, toolchain was installed in the path as below:
set (CMAKE_FIND_ROOT_PATH "/opt/hisi-linux/x86-arm/aarch64-himix100-linux")

# then set compiler
set (CMAKE_ASM_COMPILER   "/opt/hisi-linux/x86-arm/aarch64-himix100-linux/bin/aarch64-himix100-linux-gcc")
set (CMAKE_C_COMPILER     "/opt/hisi-linux/x86-arm/aarch64-himix100-linux/bin/aarch64-himix100-linux-gcc")
set (CMAKE_CXX_COMPILER   "/opt/hisi-linux/x86-arm/aarch64-himix100-linux/bin/aarch64-himix100-linux-g++")

# set searching rules for cross-compiler
set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# other needed options
set (TENGINE_TOOLCHAIN_FLAG -march=armv8-a)

# it's no need to skip OpenMP for Hi3556AV100
set (TENGINE_FORCE_SKIP_OPENMP OFF)
