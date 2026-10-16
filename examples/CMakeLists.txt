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

# generate tengine header file
file (MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tengine)
file (COPY ${CMAKE_SOURCE_DIR}/source/api/c_api.h DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/tengine)
#FILE (COPY ${CMAKE_SOURCE_DIR}/source/api/c_api.h DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
#FILE (RENAME ${CMAKE_CURRENT_BINARY_DIR}/c_api.h ${CMAKE_CURRENT_BINARY_DIR}/tengine_c_api.h)

# macro for adding test
function (TENGINE_EXAMPLE name file)
    add_executable (${name} "${CMAKE_CURRENT_SOURCE_DIR}/${file}" "${CMAKE_CURRENT_SOURCE_DIR}/common/tengine_operations.c")

    target_include_directories (${name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_include_directories (${name} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
    target_include_directories (${name} PRIVATE "${PROJECT_SOURCE_DIR}/examples/common")

    target_link_libraries (${name} PRIVATE ${CMAKE_PROJECT_NAME})

    if(MSVC)
        target_compile_definitions (${name} PRIVATE $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:NOMINMAX>)
    else()
        target_link_libraries (${name} PRIVATE m)
    endif()

    install (TARGETS ${name} DESTINATION bin)

    # add to a virtual project group
    set_property(TARGET ${name} PROPERTY FOLDER "examples")
endfunction()

# add c++ api examples
if (TENGINE_BUILD_CPP_API)
    tengine_example (cpp_tm_classification       cpp_tm_classification.cpp)
    tengine_example (cpp_tm_mobilenet_ssd        cpp_tm_mobilenet_ssd.cpp)
endif()

# add c api examples
tengine_example (tm_classification           tm_classification.c)
tengine_example (tm_classification_int8      tm_classification_int8.c)
tengine_example (tm_classification_uint8     tm_classification_uint8.c)
tengine_example (tm_efficientdet             tm_efficientdet.c)
tengine_example (tm_efficientdet_uint8       tm_efficientdet_uint8.c)
tengine_example (tm_mobilenet_ssd            tm_mobilenet_ssd.c)
tengine_example (tm_mobilenet_ssd_uint8      tm_mobilenet_ssd_uint8.cpp)
tengine_example (tm_retinaface               tm_retinaface.cpp)
tengine_example (tm_landmark                 tm_landmark.cpp)
tengine_example (tm_landmark_uint8           tm_landmark_uint8.cpp)
tengine_example (tm_mobilefacenet            tm_mobilefacenet.cpp)
tengine_example (tm_mobilefacenet_uint8      tm_mobilefacenet_uint8.cpp)
tengine_example (tm_yolov5                   tm_yolov5.cpp)
tengine_example (tm_yolofastest              tm_yolofastest.cpp)
tengine_example (tm_ultraface                tm_ultraface.cpp)



if(TENGINE_ENABLE_ACL)
    tengine_example (tm_classification_acl       tm_classification_acl.c)
    tengine_example (tm_mobilenet_ssd_acl        tm_mobilenet_ssd_acl.c)
endif()

if(TENGINE_ENABLE_CUDA)
    tengine_example (tm_classification_cuda      tm_classification_cuda.cpp)
endif()

if(TENGINE_ENABLE_OPENDLA)
    tengine_example (tm_classification_opendla      tm_classification_opendla.c)
endif()

if(TENGINE_ENABLE_TENSORRT)
    tengine_example (tm_classification_trt       tm_classification_trt.cpp)
endif()

if(TENGINE_ENABLE_TIM_VX)
    tengine_example (tm_classification_timvx     tm_classification_timvx.c)
    tengine_example (tm_landmark_timvx           tm_landmark_timvx.cpp)
    tengine_example (tm_retinaface_timvx         tm_retinaface_timvx.cpp)
    tengine_example (tm_mobilenet_ssd_uint8_timvx      tm_mobilenet_ssd_uint8_timvx.cpp)
endif()

if(TENGINE_ENABLE_VULKAN)
    tengine_example (tm_classification_vulkan       tm_classification_vulkan.c)
endif()

if (TENGINE_ARCH_ARM_82)
    tengine_example (tm_classification_fp16      tm_classification_fp16.c)
endif()


# add examples with opencv
# set(OpenCV_DIR /mnt/d/ubuntu/opencv_install/linux-armv7/lib/cmake/opencv4)
find_package(OpenCV)
message(STATUS "OpenCV_INCLUDE_DIRS:${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV_LIBS :${OpenCV_LIBS}")
if (OpenCV_FOUND)
    # macro for adding examples
    function (TENGINE_EXAMPLE_CV name file)
        add_executable (${name} "${CMAKE_CURRENT_SOURCE_DIR}/${file}" "${CMAKE_CURRENT_SOURCE_DIR}/common/tengine_operations.c")

        target_include_directories (${name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
        target_include_directories (${name} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
        target_include_directories (${name} PRIVATE "${PROJECT_SOURCE_DIR}/examples/common")
        target_include_directories (${name} PRIVATE "${PROJECT_SOURCE_DIR}/source")
        target_include_directories (${name} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/../source")

        target_link_libraries (${name} ${CMAKE_PROJECT_NAME} ${OpenCV_LIBS})
        install (TARGETS ${name} DESTINATION bin)

        # add to a virtual project group
        set_property(TARGET ${name} PROPERTY FOLDER "examples")
    endfunction()

    tengine_example_cv (tm_openpose              tm_openpose.cpp)
    tengine_example_cv (tm_yolact                tm_yolact.cpp)
    tengine_example_cv (tm_yolact_uint8          tm_yolact_uint8.cpp)
    tengine_example_cv (tm_crnn                  tm_crnn.cpp)
    tengine_example_cv (tm_alphapose             tm_alphapose.cpp)
    tengine_example_cv (tm_yolov3                tm_yolov3.cpp)
    tengine_example_cv (tm_yolov3_uint8          tm_yolov3_uint8.cpp)
    tengine_example_cv (tm_yolov3_opencl         tm_yolov3_opencl.cpp)
    tengine_example_cv (tm_yolov3_tiny           tm_yolov3_tiny.cpp)
    tengine_example_cv (tm_yolov3_tiny_opendla   tm_yolov3_tiny_opendla.cpp)
    tengine_example_cv (tm_yolov3_tiny_int8      tm_yolov3_tiny_int8.cpp)
    tengine_example_cv (tm_yolov3_tiny_uint8     tm_yolov3_tiny_uint8.cpp)
    tengine_example_cv (tm_yolov4                tm_yolov4.cpp)
    tengine_example_cv (tm_yolov4_uint8          tm_yolov4_uint8.cpp)
    tengine_example_cv (tm_yolov4_tiny           tm_yolov4_tiny.cpp)
    tengine_example_cv (tm_yolov4_tiny_uint8     tm_yolov4_tiny_uint8.cpp)
    tengine_example_cv (tm_yolov5s               tm_yolov5s.cpp)
    tengine_example_cv (tm_unet                  tm_unet.cpp)
    tengine_example_cv (tm_hrnet                 tm_hrnet.cpp)
    tengine_example_cv (tm_nanodet_m             tm_nanodet_m.cpp)
    tengine_example_cv (tm_yolox                 tm_yolox.cpp)
    tengine_example_cv (tm_yolox_int8            tm_yolox_int8.cpp)
    tengine_example_cv (tm_yolox_opendla         tm_yolox_opendla.cpp)
    tengine_example_cv (tm_yolox_darknet53       tm_yolox_darknet53.cpp)
    tengine_example_cv (tm_scrfd                 tm_scrfd.cpp)
    tengine_example_cv (tm_segformer             tm_segformer.cpp)
    tengine_example_cv (tm_seghuman              tm_seghuman.cpp)
    tengine_example_cv (tm_solov2                tm_solov2.cpp)
    tengine_example_cv (tm_apollo_smoke          tm_apollo_smoke.cpp)
    tengine_example_cv (tm_rvm                   tm_rvm.cpp)
    tengine_example_cv (tm_movenet               tm_movenet.cpp)
    tengine_example_cv (tm_picodet               tm_picodet.cpp)
    tengine_example_cv (tm_handpose              tm_handpose.cpp)
    tengine_example_cv (tm_nanodet_plus          tm_nanodet_plus.cpp)
    if(TENGINE_ENABLE_TIM_VX)
        tengine_example_cv (tm_yolov3_timvx          tm_yolov3_timvx.cpp)
        tengine_example_cv (tm_yolov4_tiny_timvx     tm_yolov4_tiny_timvx.cpp)
        tengine_example_cv (tm_yolov5s_timvx         tm_yolov5s_timvx.cpp)
        tengine_example_cv (tm_hrnet_timvx           tm_hrnet_timvx.cpp)
        tengine_example_cv (tm_nanodet_m_timvx       tm_nanodet_m_timvx.cpp)
        tengine_example_cv (tm_yolox_timvx           tm_yolox_timvx.cpp)
    endif()
    if(TENGINE_ENABLE_TENSORRT)
        tengine_example_cv (tm_scrfd_trt_fp16        tm_scrfd_trt_fp16.cpp)
    endif()

    # pthread
    if (NOT OHOS)
        include (${PROJECT_SOURCE_DIR}/cmake/libraries/pthread.cmake)
        tengine_check_lib_pthread (_has_pthread)
        if (_has_pthread)
            tengine_use_lib_pthread (${TENGINE_LITE_NAME}-static ON)
            tengine_use_lib_pthread (${TENGINE_LITE_NAME}        ON)

            tengine_example_cv(tm_pipeline_camera_preview   pipeline/test/test_camera_preview.cpp)
            tengine_example_cv(tm_pipeline_estimate_ped_distance  pipeline/test/test_pedestrian_distance_estimation.cpp)
            tengine_example_cv(tm_pipeline_enroll_face  pipeline/test/test_face_enroll.cpp)
        endif()
        unset (_has_pthread)
    endif()

else()
    message (WARNING "OpenCV not found, some examples won't be built")
endif()
