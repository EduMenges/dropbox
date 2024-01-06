cmake_minimum_required(VERSION 3.22)
project(dropbox_server)

file(GLOB_RECURSE SOURCE_FILES ${PROJECT_SOURCE_DIR}/*.cpp)
add_executable(dropbox_server ${SOURCE_FILES})

FetchContent_Declare(
        toml++
        GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
        GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(toml++)

target_include_directories(dropbox_server PRIVATE PROJECT_SOURCE_DIR)
target_include_directories(dropbox_server PRIVATE ${CMAKE_SOURCE_DIR}/common)

target_link_libraries(dropbox_server communication)
target_link_libraries(dropbox_server tomlplusplus::tomlplusplus tl::expected fmt::fmt)