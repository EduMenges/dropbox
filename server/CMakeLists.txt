cmake_minimum_required(VERSION 3.22)
project(dropbox_server)

file(GLOB_RECURSE SOURCE_FILES ${PROJECT_SOURCE_DIR}/*.cpp)
add_executable(dropbox_server ${SOURCE_FILES})

target_include_directories(dropbox_server PRIVATE PROJECT_SOURCE_DIR)
target_include_directories(dropbox_server PRIVATE ${CMAKE_SOURCE_DIR}/common)
