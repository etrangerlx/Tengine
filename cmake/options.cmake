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
#
# All Tengine build options, grouped by purpose.
# Option names and default values are part of the public build interface,
# they must stay backward compatible.

# Module options: which top level component to build
option (TENGINE_BUILD_BENCHMARK             "Build benchmark"                           ON)
option (TENGINE_BUILD_EXAMPLES              "Build examples"                            ON)
option (TENGINE_BUILD_DEMO                  "Build demos"                               OFF)
option (TENGINE_BUILD_TESTS                 "Build tests"                               OFF)
option (TENGINE_BUILD_CPP_API               "Build C++ API"                             OFF)

# Tools options: developer tools, model conversion and quantization
option (TENGINE_BUILD_CONVERT_TOOL          "Build convert tool"                        OFF)
option (TENGINE_BUILD_QUANT_TOOL            "Build quantization tool"                   OFF)

# Multithreading option
option (TENGINE_OPENMP                      "Build with OpenMP support"                 ON)

# High performance compute library standalone options
option (TENGINE_ARCH_X86_AVX                "Build AVX2 for x86"                        ON)
option (TENGINE_ARCH_ARM_82                 "Build ARM v8.2 for ARM platform"           OFF)

# Standalone HCL options
option (TENGINE_STANDALONE_HCL              "Build standalone hcl lib"                  OFF)
option (TENGINE_STANDALONE_HCL_AUTO_LOAD    "Auto load standalone hcl lib"              ON)

# Plugin options: hardware device backends
option (TENGINE_ENABLE_ACL                  "With Arm Compute Library support"          OFF)
option (TENGINE_ENABLE_CUDA                 "With nVIDIA CUDA support"                  OFF)
option (TENGINE_ENABLE_OPENCL               "With Khronos OpenCL support"               OFF)
option (TENGINE_ENABLE_OPENDLA              "With Khronos OpenDLA support"              OFF)
option (TENGINE_ENABLE_TENSORRT             "With nVIDIA TensorRT support"              OFF)
option (TENGINE_ENABLE_TIM_VX               "With VeriSilicon TIM-VX support"           OFF)
option (TENGINE_ENABLE_TORCH                "With libTorch support"                     OFF)
option (TENGINE_ENABLE_NNIE                 "With HiSilicon NNIE support"               OFF)
option (TENGINE_ENABLE_VULKAN               "With Khronos Vulkan GPU compute support"   OFF)

# Debug options
option (TENGINE_DEBUG_DATA                  "Extract feature map for each layer"        OFF)
option (TENGINE_DEBUG_TIME                  "Print execution time for each layer"       OFF)
option (TENGINE_DEBUG_MEM_STAT              "Print memory status for library"           OFF)
option (TENGINE_ENABLE_ALL_SYMBOL           "All symbol visible"                        OFF)

# Experimental options
option (TENGINE_ENABLE_MODEL_CACHE          "NPU kernel cache file option"              OFF)

# Online report
option (TENGINE_ONLINE_REPORT               "online report"                             ON)
