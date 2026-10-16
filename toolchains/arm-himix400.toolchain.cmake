# arm-himix400: Hi3520DV500 Hi3521DV200

# set cross-compiled system type, it's better not use the type which cmake cannot recognized.
set (CMAKE_SYSTEM_NAME Linux)
set (CMAKE_SYSTEM_PROCESSOR arm)

# when HiSilicon SDK was installed, toolchain was installed in the path as below:
set (CMAKE_FIND_ROOT_PATH "/opt/hisi-linux/x86-arm/arm-himix400-linux")

# then set compiler
set (CMAKE_ASM_COMPILER   "/opt/hisi-linux/x86-arm/arm-himix400-linux/bin/arm-himix400-linux-gcc")
set (CMAKE_C_COMPILER     "/opt/hisi-linux/x86-arm/arm-himix400-linux/bin/arm-himix400-linux-gcc")
set (CMAKE_CXX_COMPILER   "/opt/hisi-linux/x86-arm/arm-himix400-linux/bin/arm-himix400-linux-g++")

# set searching rules for cross-compiler
set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# other needed options
set (TENGINE_TOOLCHAIN_FLAG -march=armv7-a -mfloat-abi=softfp -mfpu=neon-vfpv4)

# it's no need to skip OpenMP for multi-core processor Hi3520DV500 or Hi3521DV200
set (TENGINE_FORCE_SKIP_OPENMP OFF)
