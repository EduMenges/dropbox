add_subdirectory(communication)

project(utils)

file(GLOB SOURCE_FILES ${PROJECT_SOURCE_DIR}/*.cpp)

add_library(utils STATIC ${SOURCE_FILES})

target_include_directories(utils PUBLIC ${PROJECT_SOURCE_DIR})

include(FetchContent)
FetchContent_Declare(
        tabulate
        GIT_REPOSITORY https://github.com/p-ranav/tabulate.git
        GIT_TAG v1.5
)
FetchContent_Declare(
        date
        GIT_REPOSITORY https://github.com/HowardHinnant/date.git
        GIT_TAG v3.0.1
)

FetchContent_MakeAvailable(tabulate)
FetchContent_MakeAvailable(date)

target_link_libraries(utils PUBLIC tabulate)
target_link_libraries(utils PRIVATE date)