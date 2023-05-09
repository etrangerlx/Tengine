[原始代码仓库](https://github.com/OAID/Tengine.git)
添加\删除远程仓库：
___
git remote -v
git remote add origin  https://github.com/OAID/Tengine.git
git remote rm origin
___

ndk-r20b编译
cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake -DANDROID_ABI="armeabi-v7a" -DANDROID_ARM_NEON=ON -DANDROID_PLATFORM=android-19 .. -G "CodeBlocks - MinGW Makefiles"
make
make install
