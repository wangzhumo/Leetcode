#Cmake Version
cmake_minimum_required(VERSION 3.11)

#project name
project(Media)

#C++
set(CMAKE_CXX_STANDARD 11)

# Add the 'Modules' folder to the search path for FindXXX.cmake files
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

#Required libraries in this source dir
# add_subdirectory(lib/entityx EXCLUDE_FROM_ALL)
# add_subdirectory(lib/glfw EXCLUDE_FROM_ALL)

#include dir
include_directories(include)

set(SRC_LIST src/main.cpp)

add_executable(Media ${SRC_LIST})