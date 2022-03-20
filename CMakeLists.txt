cmake_minimum_required(VERSION 3.6)

project(Beauty)
set(VERSION 0.1-rc)

# C++
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSION OFF)

# Beauty
add_subdirectory(src)
add_subdirectory(examples)
