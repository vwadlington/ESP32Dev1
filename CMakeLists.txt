cmake_minimum_required(VERSION 3.16)

# 1. Start the project first
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(hello_world)

# 2. Get the base version and Git hash
file(STRINGS "version.txt" BASE_VER)
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

# 3. Apply the custom version to the project AFTER it is defined
if(GIT_HASH)
    set(APP_PROJECT_VER "${BASE_VER}-g${GIT_HASH}")
else()
    set(APP_PROJECT_VER "${BASE_VER}")
endif()

# This is the "magic" line that forces the app to use our version
set(PROJECT_VER "${APP_PROJECT_VER}")

message(STATUS "******************************************")
message(STATUS "FINAL APP VERSION: ${PROJECT_VER}")
message(STATUS "******************************************")