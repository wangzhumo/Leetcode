#Cmake Version
cmake_minimum_required(VERSION 3.11)

#project name
project(Media)

#C++
set(CMAKE_CXX_STANDARD 11)

# Add the 'Modules' folder to the search path for FindXXX.cmake files
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

set(CMAKE_CXX_FLAGS “${CMAKE_CXX_FLAGS} -g”)
set(CMAKE_BUILD_TYPE Debug CACHE STRING "set build type to debug")


# Add all binary files
aux_source_directory(src SRC_LIST)
aux_source_directory(src/sdls SDL_SRC_LIST)
aux_source_directory(src/play PLAY_SRC_LIST)
set(ALL_SOURCE ${SRC_LIST} ${SDL_SRC_LIST} ${PLAY_SRC_LIST})
add_executable(Media ${ALL_SOURCE})

# Find SDL/FFmpeg include directories and link libraries.
find_package(SDL2 REQUIRED)
find_package(FFMPEG COMPONENTS avcodec avformat avutil swscale swresample avfilter)


# Include module pkg-config for CMake.
INCLUDE(FindPkgConfig)

# Include dir
set(LIBRARIES
        ${SDL2_LIBRARY}
        ${FFMPEG_LIBRARIES})
set(INCLUDES
        include
        ${SDL2_INCLUDE_DIR}
        ${FFMPEG_INCLUDE_DIRS})
include_directories(${INCLUDES})

# Link dirs
target_link_libraries(Media ${LIBRARIES})