

cmake_minimum_required(VERSION 3.16)
project(THREADPOOL)

#set(CMAKE_CXX_STANDARD 17) #2a
set(CMAKE_CXX_STANDARD 20) #2a

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

include_directories(${PROJECT_SOURCE_DIR}/include)

aux_source_directory(${PROJECT_SOURCE_DIR}/src SRC_THRPOOL)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

set(CMAKE_BUILD_TYPE Debug)

add_subdirectory(example)

