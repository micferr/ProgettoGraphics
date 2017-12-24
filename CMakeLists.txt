cmake_minimum_required (VERSION 3.5)

project (graphics17a_project)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)
# set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_BUILD_TYPE Release)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})


if(APPLE)
    add_definitions(-Wno-missing-braces)
endif(APPLE)
if(WIN32)
    add_definitions(/D_CRT_SECURE_NO_WARNINGS /wd4018 /wd4244 /wd4305 /wd4800 /wd4267)
endif(WIN32)

add_definitions(-DYOBJ_NO_IMAGE -DYGLTF_NO_IMAGE -DYSCN_NO_IMAGE)
add_definitions(-DYGL_IMAGEIO=1 -DYGL_GLTF=1 )

add_library(yocto_gl
	src/grammar.h
	src/node.h

	src/yocto/yocto_gl.h
	src/yocto/yocto_gl.cpp
	src/yocto/ext/stb_image.cpp
)

add_executable(test_main src/test_main.cpp)
target_link_libraries(test_main yocto_gl)