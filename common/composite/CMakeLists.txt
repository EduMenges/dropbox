project(composite)

file(GLOB_RECURSE SOURCE_FILES ${PROJECT_SOURCE_DIR}/*.cpp)

add_library(composite STATIC ${SOURCE_FILES})

target_link_libraries(composite PUBLIC utils networking communication)