project(communication)

file(GLOB_RECURSE SOURCE_FILES ${PROJECT_SOURCE_DIR}/*.cpp)

add_library(communication STATIC ${SOURCE_FILES})

target_link_libraries(communication PUBLIC utils networking fmt::fmt)
