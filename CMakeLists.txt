cmake_minimum_required(VERSION 3.16)

# 1. Pull in the IDF framework
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# 2. Get the base version from your version.txt
file(STRINGS "version.txt" BASE_VER)

# 3. Manually grab that 7-character hash you saw in the terminal
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

# 4. Combine them (Result looks like: 1.0.0-g7a1b2c3)
if(GIT_HASH)
    set(APP_PROJECT_VER "${BASE_VER}-g${GIT_HASH}")
else()
    set(APP_PROJECT_VER "${BASE_VER}")
endif()

# 5. Define the project
project(hello_world)