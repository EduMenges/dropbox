add_subdirectory(communication)

project(utils)

file(GLOB SOURCE_FILES ${PROJECT_SOURCE_DIR}/*.cpp)

add_library(utils STATIC ${SOURCE_FILES})

target_include_directories(utils PUBLIC ${PROJECT_SOURCE_DIR})
