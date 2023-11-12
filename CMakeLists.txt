cmake_minimum_required(VERSION 3.22)
project(dropbox)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_CLIENT "Builds the client project" ON)
option(BUILD_SERVER "Builds the server project" ON)

if (BUILD_CLIENT)
	add_subdirectory(client)
endif()

if(BUILD_SERVER)
	add_subdirectory(server)
endif()
