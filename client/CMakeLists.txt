cmake_minimum_required(VERSION 3.22)
project(dropbox_client)

file(GLOB_RECURSE SOURCE_FILES ${PROJECT_SOURCE_DIR}/*.cpp)
add_executable(dropbox_client ${SOURCE_FILES})

target_include_directories(dropbox_client PRIVATE PROJECT_SOURCE_DIR)
target_include_directories(dropbox_client PRIVATE ${CMAKE_SOURCE_DIR}/common)
