add_subdirectory(communication)

project(utils)

file(GLOB SOURCE_FILES *.cpp)

add_library(utils ${SOURCE_FILES})