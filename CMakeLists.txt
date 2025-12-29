cmake_minimum_required(VERSION 3.16)

# Include the ESP-IDF project logic first
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Read the version from your file
file(STRINGS "version.txt" PROJECT_VER)

# Set the version (This will now allow the build system to append the git hash)
set(APP_PROJECT_VER "${PROJECT_VER}")

# Define the project
project(hello_world)