cmake_minimum_required(VERSION 3.27)

project(dropbox)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(dropbox Dropbox.cpp)
target_include_directories(dropbox PRIVATE PROJECT_SOURCE_DIR)
