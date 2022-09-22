cmake_minimum_required(VERSION 3.18.0)

project(file-diff C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -ggdb")

set(srcs "layer.cc") 

#add_library(syncfile STATIC ${srcs})

add_executable(layer ${srcs})
