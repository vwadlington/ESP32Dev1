# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

# 1. Read the version from the file in the same folder
file(STRINGS "version.txt" PROJECT_VER)
# 2. Tell ESP-IDF to use this version for the app metadata
set(APP_PROJECT_VER "${PROJECT_VER}")

set(COMPONENTS main)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(hello_world)
cmake_minimum_required(VERSION 3.16)