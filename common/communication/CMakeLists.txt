project(communication)

file(GLOB_RECURSE SOURCE_FILES ${PROJECT_SOURCE_DIR}/*.cpp)

add_library(communication STATIC ${SOURCE_FILES})
target_include_directories(communication PUBLIC "../")