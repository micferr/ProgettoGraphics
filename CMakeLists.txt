cmake_minimum_required (VERSION 3.5)

project (graphics17a_project)

option(YOCTO_OPENGL "Build OpenGL apps" ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)
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

if(YOCTO_OPENGL)
    add_definitions(-DYGL_OPENGL=1)
else(YOCTO_OPENGL)
    add_definitions(-DYGL_OPENGL=0)
endif(YOCTO_OPENGL)

add_definitions(-DYOBJ_NO_IMAGE -DYGLTF_NO_IMAGE -DYSCN_NO_IMAGE)
add_definitions(-DYGL_IMAGEIO=1 -DYGL_GLTF=1 )

find_package(OpenGL REQUIRED)

if(APPLE)
    include_directories(/usr/local/include)
    link_directories(/usr/local/lib)
    find_library(GLFW_LIBRARY NAMES glfw3 glfw PATHS /usr/local/lib)
endif(APPLE)
if(WIN32)
    include_directories(${CMAKE_SOURCE_DIR}/src/apps/w32/include)
    link_directories(${CMAKE_SOURCE_DIR}/src/apps/w32/lib-vc2015)
    find_library(GLFW_LIBRARY NAMES glfw3dll PATHS ${CMAKE_SOURCE_DIR}/src/apps/w32/lib-vc2015)
    find_library(GLEW_LIBRARIES NAMES glew32 PATHS ${CMAKE_SOURCE_DIR}/src/apps/w32/lib-vc2015)
endif(WIN32)
if(UNIX AND NOT APPLE)
    include_directories(/usr/include /usr/local/include)
    find_library(GLFW_LIBRARY NAMES glfw3 glfw PATHS /usr/lib /usr/local/lib64 /usr/lib64 /usr/local/lib /usr/lib/x86_64-linux-gnu)
    find_package(GLEW REQUIRED)
endif(UNIX AND NOT APPLE)

add_library(yocto_gl
	src/yocto/yocto_gl.h
	src/yocto/yocto_gl.cpp
	src/yocto/ext/stb_image.cpp
	src/yocto/ext/imgui/imgui.cpp 
	src/yocto/ext/imgui/imgui_draw.cpp 
    src/yocto/ext/imgui/imgui_impl_glfw_gl3.cpp
)
target_link_libraries(yocto_gl ${OPENGL_gl_LIBRARY} ${GLFW_LIBRARY} ${GLEW_LIBRARIES})

add_library(poly2tri
    src/poly2tri/poly2tri/poly2tri.h
    src/poly2tri/poly2tri/common/utils.h
    src/poly2tri/poly2tri/common/shapes.h
    src/poly2tri/poly2tri/common/shapes.cc

    src/poly2tri/poly2tri/sweep/advancing_front.h
    src/poly2tri/poly2tri/sweep/advancing_front.cc
    src/poly2tri/poly2tri/sweep/cdt.h
    src/poly2tri/poly2tri/sweep/cdt.cc
    src/poly2tri/poly2tri/sweep/sweep.h
    src/poly2tri/poly2tri/sweep/sweep.cc
    src/poly2tri/poly2tri/sweep/sweep_context.h
    src/poly2tri/poly2tri/sweep/sweep_context.cc
)

add_library(clipper
	src/clipper/clipper.hpp
	src/clipper/clipper.cpp
)

add_executable(main src/main.cpp
	src/grammar.h
	src/node.h
	src/building_utils.h
	src/geom_utils.h
	src/prob_utils.h
	src/yocto_utils.h
)
target_link_libraries(main yocto_gl poly2tri clipper)

add_executable(test_main src/test_main.cpp
	src/grammar.h
	src/node.h
	src/building_utils.h
	src/geom_utils.h
	src/prob_utils.h
	src/yocto_utils.h
)
target_link_libraries(test_main yocto_gl poly2tri clipper)

add_executable(ytestgen src/apps/ytestgen.cpp src/yocto/yocto_gl.h)
add_executable(ytrace src/apps/ytrace.cpp src/yocto/yocto_gl.h)
add_executable(yscnproc src/apps/yscnproc.cpp src/yocto/yocto_gl.h)
add_executable(yimproc src/apps/yimproc.cpp src/yocto/yocto_gl.h)

target_link_libraries(ytestgen yocto_gl)
target_link_libraries(ytrace yocto_gl)
target_link_libraries(yscnproc yocto_gl)
target_link_libraries(yimproc yocto_gl)

if(YOCTO_OPENGL)
    add_executable(yview src/apps/yview.cpp src/yocto/yocto_gl.h)
    add_executable(yitrace src/apps/yitrace.cpp src/yocto/yocto_gl.h)
    add_executable(yimview src/apps/yimview.cpp src/yocto/yocto_gl.h)

    target_link_libraries(yview yocto_gl)
    target_link_libraries(yitrace yocto_gl)
    target_link_libraries(yimview yocto_gl)
endif(YOCTO_OPENGL)