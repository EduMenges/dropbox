cmake_minimum_required(VERSION 3.22)
project(dropbox LANGUAGES CXX)

include(CheckIncludeFiles)
CHECK_INCLUDE_FILES("sys/socket.h;arpa/inet.h;netinet/in.h" SOCKET_HEADERS)

IF (SOCKET_HEADERS)
	MESSAGE("Headers for POSIX sockets were found.")
ELSE ()
	MESSAGE(FATAL_ERROR "Headers for POSIX sockets were not found.")
ENDIF ()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_CLIENT "Builds the client project" ON)
option(BUILD_SERVER "Builds the server project" ON)

if (MSVC)
	add_compile_options(/W4)
else()
	add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_subdirectory(common)
link_libraries(utils)

if (BUILD_CLIENT)
	add_subdirectory(client)
endif()

if(BUILD_SERVER)
	add_subdirectory(server)
endif()
