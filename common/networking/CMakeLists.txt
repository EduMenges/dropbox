project(networking)

file(GLOB_RECURSE SOURCE_FILES ${PROJECT_SOURCE_DIR}/*.cpp)

add_library(networking STATIC ${SOURCE_FILES})

target_link_libraries(networking PUBLIC utils)
