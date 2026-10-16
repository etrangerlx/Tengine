# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# License); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# AS IS BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
# Copyright (c) 2021, OPEN AI LAB
# Author: lswang@openailab.com
#

# Compilers:
#
# - TENGINE_COMPILER_GCC                    - GNU compiler (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
# - TENGINE_COMPILER_CLANG                  - Clang-compatible compiler (CMAKE_CXX_COMPILER_ID MATCHES "Clang" - Clang or AppleClang, see CMP0025)
# - TENGINE_COMPILER_ICC                    - Intel compiler
# - TENGINE_COMPILER_MSVC                   - MSVC, Microsoft Visual Compiler (CMake variable)
# - TENGINE_COMPILER_MINGW TENGINE_COMPILER_CYGWIN  - MINGW / CYGWIN / CMAKE_COMPILER_IS_MINGW / CMAKE_COMPILER_IS_CYGWIN (CMake original variables)
#
# CPU Platforms String:
# - X86                                     - world wide architecture, u know who, x86 or amd64
# - ARM                                     - Advanced RISC Machine architecture, a.k.a ARM architecture
# - PPC64                                   - PowerPC
# - PPC64LE                                 - PowerPC Little Edition
# - MIPS                                    - Million Instructions Per Second architecture, a.k.a MIPS architecture
#
# OS:
# - TENGINE_SYSTEM_WINDOWS                  - Windows |  MinGW
# - TENGINE_SYSTEM_LINUX                    -  Linux  | Android
# - TENGINE_SYSTEM_ANDROID                  - Android
# - TENGINE_SYSTEM_IOS                      -   iOS
# - TENGINE_SYSTEM_APPLE    - MacOSX | iOS
# - TENGINE_SYSTEM_OHOS                     - Harmony OS


# check target cpu
if (TENGINE_SKIP_TARGET_PROCESSOR_CHECK)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "amd64.*|x86_64.*|AMD64.*")
    set (TENGINE_TARGET_PROCESSOR        "X86"   CACHE INTERNAL "" FORCE)
    set (TENGINE_TARGET_PROCESSOR_32Bit  FALSE   CACHE INTERNAL "" FORCE)
    set (TENGINE_TARGET_PROCESSOR_64Bit  TRUE    CACHE INTERNAL "" FORCE)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "i686.*|i386.*|x86.*")
    set (TENGINE_TARGET_PROCESSOR        "X86"   CACHE INTERNAL "" FORCE)
    set (TENGINE_TARGET_PROCESSOR_32Bit  TRUE    CACHE INTERNAL "" FORCE)
    set (TENGINE_TARGET_PROCESSOR_64Bit  FALSE   CACHE INTERNAL "" FORCE)
elseif ((IOS AND CMAKE_OSX_ARCHITECTURES MATCHES "arm") OR (CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64.*|AARCH64.*|arm64.*|ARM64.*)") OR (APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64.*|AARCH64.*|arm64.*|ARM64.*)"))
    set (TENGINE_TARGET_PROCESSOR        "ARM"   CACHE INTERNAL "" FORCE)
    set (TENGINE_TARGET_PROCESSOR_32Bit  FALSE   CACHE INTERNAL "" FORCE)
    set (TENGINE_TARGET_PROCESSOR_64Bit  TRUE    CACHE INTERNAL "" FORCE)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm.*|ARM.*)")
    set (TENGINE_TARGET_PROCESSOR        "ARM"   CACHE INTERNAL "" FORCE)
    set (TENGINE_TARGET_PROCESSOR_32Bit  TRUE    CACHE INTERNAL "" FORCE)
    set (TENGINE_TARGET_PROCESSOR_64Bit  FALSE   CACHE INTERNAL "" FORCE)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc)64le")
    set (TENGINE_TARGET_PROCESSOR        "PPCLE" CACHE INTERNAL "" FORCE)
    set (TENGINE_TARGET_PROCESSOR_32Bit  FALSE   CACHE INTERNAL "" FORCE)
    set (TENGINE_TARGET_PROCESSOR_64Bit  TRUE    CACHE INTERNAL "" FORCE)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc)64")
    set (TENGINE_TARGET_PROCESSOR        "PPC"   CACHE INTERNAL "" FORCE)
    set (TENGINE_TARGET_PROCESSOR_32Bit  FALSE   CACHE INTERNAL "" FORCE)
    set (TENGINE_TARGET_PROCESSOR_64Bit  TRUE    CACHE INTERNAL "" FORCE)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^(mips.*|MIPS.*|mips64.*)")
    set (TENGINE_TARGET_PROCESSOR        "MIPS"  CACHE INTERNAL "" FORCE)
    set (TENGINE_TARGET_PROCESSOR_32Bit  FALSE   CACHE INTERNAL "" FORCE)
    set (TENGINE_TARGET_PROCESSOR_64Bit  TRUE    CACHE INTERNAL "" FORCE)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^(rv64.*|RV64.*|riscv64.*)")
    set (TENGINE_TARGET_PROCESSOR        "lp64dv" CACHE INTERNAL "" FORCE)
    set (TENGINE_TARGET_PROCESSOR_32Bit  FALSE    CACHE INTERNAL "" FORCE)
    set (TENGINE_TARGET_PROCESSOR_64Bit  TRUE     CACHE INTERNAL "" FORCE)
else()
    if (NOT TENGINE_SUPPRESS_TARGET_PROCESSOR_CHECK)
        message (WARNING "TENGINE: Unrecognized target processor configuration. " ${CMAKE_SYSTEM_PROCESSOR})
    endif()
endif()


# Workaround for 32-bit operating systems on x86_64
if ((CMAKE_SIZEOF_VOID_P EQUAL 4) AND (TENGINE_TARGET_PROCESSOR MATCHES "X86") AND (NOT TENGINE_FORCE_BUILD_X86_64))
    if (NOT TENGINE_SUPPRESS_TARGET_PROCESSOR_CHECK)
        message (WARNING "TENGINE: 32Bit target OS is detected. Assume 32-bit compilation mode.")
    endif()
    if (TENGINE_TARGET_PROCESSOR_64Bit)
        set (TENGINE_TARGET_PROCESSOR_64Bit  FALSE   CACHE INTERNAL "" FORCE)
        set (TENGINE_TARGET_PROCESSOR_32Bit  TRUE    CACHE INTERNAL "" FORCE)
    endif()
endif()

# Workaround for 32-bit operating systems on aarch64 processor
if ((CMAKE_SIZEOF_VOID_P EQUAL 4) AND (TENGINE_TARGET_PROCESSOR MATCHES "ARM") AND (NOT TENGINE_FORCE_BUILD_AARCH64))
    if (NOT TENGINE_SUPPRESS_TARGET_PROCESSOR_CHECK)
        message (STATUS "TENGINE: 32Bit target OS is detected. Assume 32-bit compilation mode.")
    endif()
    if (TENGINE_TARGET_PROCESSOR_64Bit)
        set (TENGINE_TARGET_PROCESSOR_32Bit  TRUE    CACHE INTERNAL "" FORCE)
        set (TENGINE_TARGET_PROCESSOR_64Bit  FALSE   CACHE INTERNAL "" FORCE)
    endif()
endif()



# Check which compiler
# GCC, the GNU Compiler Collection
if (NOT DEFINED TENGINE_COMPILER_GCC AND CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set (TENGINE_COMPILER_GCC   TRUE CACHE INTERNAL "" FORCE)
endif()

# Clang, C Language Family Frontend for LLVM
if(NOT DEFINED TENGINE_COMPILER_CLANG AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")  # Clang or AppleClang (see CMP0025)
    set (TENGINE_COMPILER_CLANG TRUE CACHE INTERNAL "" FORCE)
endif()

# ICC, Intel C++ Compiler
if (UNIX)
    if     (__ICL)
        set (TENGINE_COMPILER_ICC   __ICL                   CACHE INTERNAL "" FORCE)
    elseif (__ICC)
        set (TENGINE_COMPILER_ICC   __ICC                   CACHE INTERNAL "" FORCE)
    elseif (__ECL)
        set (TENGINE_COMPILER_ICC   __ECL                   CACHE INTERNAL "" FORCE)
    elseif (__ECC)
        set (TENGINE_COMPILER_ICC   __ECC                   CACHE INTERNAL "" FORCE)
    elseif (__INTEL_COMPILER)
        set (TENGINE_COMPILER_ICC   __INTEL_COMPILER)
    elseif (CMAKE_C_COMPILER MATCHES "icc")
        set (TENGINE_COMPILER_ICC   icc_matches_c_compiler  CACHE INTERNAL "" FORCE)
    endif()
endif()

if (MSVC AND CMAKE_C_COMPILER MATCHES "icc|icl")
    set (TENGINE_COMPILER_ICC  __INTEL_COMPILER_FOR_WINDOWS CACHE INTERNAL "" FORCE)
endif()

# MSVC, Microsoft Visual C++ Compiler
if (MSVC)
    set (TENGINE_COMPILER_MSVC      TRUE CACHE INTERNAL "" FORCE)
endif()

# MinGW, Minimalist GNU for Windows
if (MINGW OR CMAKE_COMPILER_IS_MINGW)
    set (TENGINE_COMPILER_GCC       TRUE CACHE INTERNAL "" FORCE)
    set (TENGINE_COMPILER_MINGW     TRUE CACHE INTERNAL "" FORCE)
endif()

# Cygwin
if (CYGWIN OR CMAKE_COMPILER_IS_CYGWIN)
    set (TENGINE_COMPILER_GCC       TRUE CACHE INTERNAL "" FORCE)
    set (TENGINE_COMPILER_CYGWIN    TRUE CACHE INTERNAL "" FORCE)
endif()



# check system
if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    set (TENGINE_SYSTEM     "Windows"           CACHE INTERNAL "" FORCE)
elseif(UNIX AND NOT APPLE)
    if(CMAKE_SYSTEM_NAME MATCHES "Android")
        set (TENGINE_SYSTEM "Android"           CACHE INTERNAL "" FORCE)
    elseif(OHOS)
        set (TENGINE_SYSTEM "Harmony OS"        CACHE INTERNAL "" FORCE)
    elseif(CMAKE_SYSTEM_NAME MATCHES ".*Linux")
        set (TENGINE_SYSTEM "Linux"             CACHE INTERNAL "" FORCE)
    elseif(CMAKE_SYSTEM_NAME MATCHES "kFreeBSD.*")
        set (TENGINE_SYSTEM "FreeBSD"           CACHE INTERNAL "" FORCE)
    elseif(CMAKE_SYSTEM_NAME MATCHES "DragonFly.*|FreeBSD")
        set (TENGINE_SYSTEM "FreeBSD"           CACHE INTERNAL "" FORCE)
    elseif(CMAKE_SYSTEM_NAME MATCHES "kNetBSD.*|NetBSD.*")
        set (TENGINE_SYSTEM "NetBSD"            CACHE INTERNAL "" FORCE)
    elseif(CMAKE_SYSTEM_NAME MATCHES "kOpenBSD.*|OpenBSD.*")
        set (TENGINE_SYSTEM "OpenBSD"           CACHE INTERNAL "" FORCE)
    elseif(CMAKE_SYSTEM_NAME MATCHES "SYSV5.*")
        set (TENGINE_SYSTEM "System V"          CACHE INTERNAL "" FORCE)
    elseif(CMAKE_SYSTEM_NAME MATCHES "Solaris.*")
        set (TENGINE_SYSTEM "Solaris"           CACHE INTERNAL "" FORCE)
    elseif(CMAKE_SYSTEM_NAME MATCHES "HP-UX.*")
        set (TENGINE_SYSTEM "HP-UX"             CACHE INTERNAL "" FORCE)
    elseif(CMAKE_SYSTEM_NAME MATCHES "AIX.*")
        set (TENGINE_SYSTEM "AIX"               CACHE INTERNAL "" FORCE)
    elseif(CMAKE_SYSTEM_NAME MATCHES "Minix.*")
        set (TENGINE_SYSTEM "Minix"             CACHE INTERNAL "" FORCE)
    endif()
elseif(APPLE)
    if(CMAKE_SYSTEM_NAME MATCHES ".*Darwin.*")
        set (TENGINE_SYSTEM "Darwin"            CACHE INTERNAL "" FORCE)
    elseif(CMAKE_SYSTEM_NAME MATCHES ".*MacOS.*")
        set (TENGINE_SYSTEM "MacOS"             CACHE INTERNAL "" FORCE)
    else()
        set (TENGINE_SYSTEM "Apple"             CACHE INTERNAL "" FORCE)
    endif()
endif ()



# Check C/C++ standard support
include (CheckLanguage)
include (CheckCCompilerFlag)
include (CheckCXXCompilerFlag)

# C99 is the base required standard
set (CMAKE_C_STANDARD 99)
set (CMAKE_C_STANDARD_REQUIRED TRUE)
set (CMAKE_C_EXTENSIONS ON)

# Try to enable C11 standard
if (TENGINE_USE_LASTEST_C_STANDARD OR TENGINE_USE_LASTEST_STANDARD)
    check_c_compiler_flag ("-std=c11" TENGINE_ENV_HAS_C11)

    if (TENGINE_ENV_HAS_C11)
        set (CMAKE_C_STANDARD 11)
    endif()
endif()



# C++11 is the base required standard
if (TENGINE_ENABLE_TORCH)
    set (CMAKE_CXX_STANDARD 17)
else()
    set (CMAKE_CXX_STANDARD 11)
endif()
set (CMAKE_CXX_STANDARD_REQUIRED TRUE)
set (CMAKE_CXX_EXTENSIONS OFF)

# Try to enable C++14 standard
if (TENGINE_USE_LASTEST_CXX_STANDARD OR TENGINE_USE_LASTEST_STANDARD)
    check_cxx_compiler_flag ("-std=c++14" TENGINE_ENV_HAS_CXX14)

    if (TENGINE_ENV_HAS_CXX14)
        set (CMAKE_CXX_STANDARD 14)
    endif()

    check_cxx_compiler_flag ("-std=c++17" TENGINE_ENV_HAS_CXX17)
endif()
