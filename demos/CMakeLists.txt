# generate tengine header file
file (MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tengine)
file (COPY ${CMAKE_SOURCE_DIR}/source/api/c_api.h DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/tengine)

find_package(OpenCV)

function (TENGINE_DEMO name)
    add_executable (${name})

    foreach(file IN LISTS ARGN)
        target_sources(${name} PRIVATE ${file})
    endforeach()

    target_include_directories (${name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_include_directories (${name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/algorithm")
    target_include_directories (${name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/utilities")
    target_include_directories (${name} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
    target_include_directories (${name} PRIVATE "${OpenCV_INCLUDE_DIRS}")

    target_link_libraries (${name} PRIVATE ${CMAKE_PROJECT_NAME} ${OpenCV_LIBS})

    if(MSVC)
        target_compile_definitions (${name} PRIVATE $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:NOMINMAX>)

        # add to a virtual project group
        set_property(TARGET ${name} PROPERTY FOLDER "demos")
    else()
        target_link_libraries (${name} PRIVATE m)
    endif()

    install (TARGETS ${name} DESTINATION demo)
endfunction()


if(OpenCV_FOUND)
    tengine_demo (demo_yolo_camera      demo_yolo_camera.cpp     algorithm/yolo.cpp algorithm/yolo_layer.cpp utilities/timer.cc)
    tengine_demo (demo_scrfd_camera     demo_scrfd_camera.cc     algorithm/scrfd.cc utilities/letterbox.cc utilities/permute.cc utilities/nms.cc utilities/iou.cc utilities/timer.cc)
    tengine_demo (demo_scrfd_image      demo_scrfd_image.cc      algorithm/scrfd.cc utilities/letterbox.cc utilities/permute.cc utilities/nms.cc utilities/iou.cc utilities/timer.cc)
    tengine_demo (demo_arcface_image    demo_arcface_image.cc    algorithm/scrfd.cc algorithm/arcface.cc utilities/permute.cc utilities/timer.cc utilities/affine.cc utilities/letterbox.cc utilities/nms.cc utilities/iou.cc utilities/distance.cc)
    tengine_demo (demo_scrfd_list       demo_scrfd_list.cc       algorithm/scrfd.cc utilities/letterbox.cc utilities/iou.cc utilities/nms.cc utilities/permute.cc utilities/timer.cc)
    tengine_demo (demo_arcface_list     demo_arcface_list.cc     algorithm/arcface.cc utilities/affine.cc utilities/distance.cc utilities/permute.cc utilities/timer.cc)
    tengine_demo (demo_centerface_image demo_centerface_image.cc algorithm/centerface.cc utilities/letterbox.cc utilities/permute.cc utilities/nms.cc utilities/iou.cc utilities/timer.cc)
    tengine_demo (demo_centerface_list  demo_centerface_list.cc  algorithm/centerface.cc utilities/letterbox.cc utilities/permute.cc utilities/nms.cc utilities/iou.cc utilities/timer.cc)
else()
    message(WARNING "OpenCV not found, some DEMO won't be built")
endif()
