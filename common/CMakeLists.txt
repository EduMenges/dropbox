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
set(USE_CPP17 ON)
FetchContent_MakeAvailable(tabulate)

FetchContent_Declare(
        date
        GIT_REPOSITORY https://github.com/HowardHinnant/date.git
        GIT_TAG v3.0.1
)
FetchContent_MakeAvailable(date)

FetchContent_Declare(
        cereal
        GIT_REPOSITORY https://github.com/USCiLab/cereal.git
        GIT_TAG v1.3.2
)
set(BUILD_DOC OFF)
set(BUILD_SANDBOX OFF)
set(SKIP_PERFORMANCE_COMPARISON ON)
FetchContent_MakeAvailable(cereal)

FetchContent_Declare(
        expected
        GIT_REPOSITORY https://github.com/TartanLlama/expected.git
        GIT_TAG v1.1.0
)
set(EXPECTED_BUILD_PACKAGE OFF)
FetchContent_MakeAvailable(expected)

FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 10.2.1
)
FetchContent_MakeAvailable(fmt)

target_link_libraries(utils PUBLIC tabulate)
target_link_libraries(utils PRIVATE date)
target_link_libraries(utils PUBLIC cereal)

add_subdirectory(communication)
add_subdirectory(networking)
add_subdirectory(composite)