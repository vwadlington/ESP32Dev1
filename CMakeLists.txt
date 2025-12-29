cmake_minimum_required(VERSION 3.16)

# 1. Get version and hash BEFORE anything else
file(STRINGS "version.txt" BASE_VER)
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

set(FULL_VER "${BASE_VER}-g${GIT_HASH}")

# 2. Set these as CACHE variables (this forces them into the build system's memory)
set(PROJECT_VER "${FULL_VER}" CACHE STRING "" FORCE)
set(APP_PROJECT_VER "${FULL_VER}" CACHE STRING "" FORCE)

# 3. Now load the project
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(hello_world)

message(STATUS "--------------------------------------------")
message(STATUS "FORCING BINARY VERSION: ${FULL_VER}")
message(STATUS "--------------------------------------------")