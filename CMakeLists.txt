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

aux_source_directory(src SRC_LIST)
#set(SRC_LIST .src/*)

add_executable(Media ${SRC_LIST})

find_package(SDL2 REQUIRED)
#find_package(SDL2_image REQUIRED)
#find_package(SDL2_ttf REQUIRED)


include_directories(
        ${SDL2_INCLUDE_DIR}
        ${SDL2_IMAGE_INCLUDE_DIR}
        ${SDL2_TTF_INCLUDE_DIR}
)

target_link_libraries(Media
        ${SDL2_LIBRARY}
        ${SDL2_IMAGE_LIBRARIES}
        ${SDL2_TTF_LIBRARIES}
        )