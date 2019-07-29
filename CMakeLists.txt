#Cmake Version
cmake_minimum_required(VERSION 3.11)

#project name
project(Media)

#C++
set(CMAKE_CXX_STANDARD 11)

# Add the 'Modules' folder to the search path for FindXXX.cmake files
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

set(CMAKE_CXX_FLAGS “${CMAKE_CXX_FLAGS} -g”)
set(CMAKE_BUILD_TYPE "Debug") 
#Required libraries in this source dir
# add_subdirectory(lib/entityx EXCLUDE_FROM_ALL)
# add_subdirectory(lib/glfw EXCLUDE_FROM_ALL)

aux_source_directory(src SRC_LIST)

add_executable(Media ${SRC_LIST})

find_package(SDL2 REQUIRED)


#include dir
include_directories(
        include
        ${SDL2_INCLUDE_DIR}
)

target_link_libraries(
        Media
        ${SDL2_LIBRARY}
)