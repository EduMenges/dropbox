project(communication)

file(GLOB_RECURSE SOURCE_FILES ${PROJECT_SOURCE_DIR}/*.cpp)

add_library(communication STATIC ${SOURCE_FILES})

target_include_directories(communication PUBLIC "../")

include(FetchContent)

FetchContent_Declare(
        cereal
        GIT_REPOSITORY https://github.com/USCiLab/cereal.git
        GIT_TAG v1.3.2
)
set(SKIP_PERFORMANCE_COMPARISON ON)
FetchContent_MakeAvailable(cereal)

target_link_libraries(communication PUBLIC cereal)
