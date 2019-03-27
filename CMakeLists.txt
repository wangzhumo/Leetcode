cmake_minimum_required(VERSION 3.13)
project(leetcode)

set(CMAKE_CXX_STANDARD 14)

aux_source_directory(./src SOURCE_DIR)

add_executable(leetcode ${SOURCE_DIR})