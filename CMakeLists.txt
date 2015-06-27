cmake_minimum_required (VERSION 3.2)

# build type defaults to debug
if(NOT DEFINED CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
endif(NOT DEFINED CMAKE_BUILD_TYPE)
# set default compiler flags to enable C++14
if(NOT DEFINED CMAKE_CXX_FLAGS)
	set(CMAKE_CXX_FLAGS -std=c++14 CACHE STRING "Flags used by the compiler during all build types.")
endif(NOT DEFINED CMAKE_CXX_FLAGS)

project(transode CXX C)

#FIXME: search for ffmpeg libs to set include dir and target libraries

add_executable(transcode main.cpp transcode.cpp)
target_link_libraries(transcode avformat avcodec avutil avfilter swscale)
