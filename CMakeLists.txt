cmake_minimum_required (VERSION 3.2)

project(transode CXX C)

#FIXME: search for ffmpeg libs to set include dir and target libraries

add_executable(transcode main.cpp transcode.cpp)
target_link_libraries(transcode avformat avcodec avutil avfilter swscale)
