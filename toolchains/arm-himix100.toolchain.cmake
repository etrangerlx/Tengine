# arm-himix100: Hi3516DV200 Hi3516EV200 Hi3516EV300 Hi3518EV300; Hi3556V200 Hi3559V200

# set cross-compiled system type, it's better not use the type which cmake cannot recognized.
set (CMAKE_SYSTEM_NAME Linux)
set (CMAKE_SYSTEM_PROCESSOR arm)

# when HiSilicon SDK was installed, toolchain was installed in the path as below:
set (CMAKE_FIND_ROOT_PATH "/opt/hisi-linux/x86-arm/arm-himix100-linux")

# then set compiler
set (CMAKE_ASM_COMPILER   "/opt/hisi-linux/x86-arm/arm-himix100-linux/bin/arm-himix100-linux-gcc")
set (CMAKE_C_COMPILER     "/opt/hisi-linux/x86-arm/arm-himix100-linux/bin/arm-himix100-linux-gcc")
set (CMAKE_CXX_COMPILER   "/opt/hisi-linux/x86-arm/arm-himix100-linux/bin/arm-himix100-linux-g++")

# set searching rules for cross-compiler
set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# other needed options
set (TENGINE_TOOLCHAIN_FLAG -march=armv7-a -mfloat-abi=softfp -mfpu=neon-vfpv4)

# it may be a good choice for this low cost series products to skip OpenMP check
set (TENGINE_FORCE_SKIP_OPENMP ON)
