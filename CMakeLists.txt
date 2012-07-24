cmake_minimum_required(VERSION 2.8)
enable_testing()
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)
SET_PROPERTY(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER CMake)



##############################################################################
## Project
##############################################################################
project("SFJ" CXX)
set(PROJECT_NAME sfj)



##############################################################################
## Common
##############################################################################
add_subdirectory(black_label)
add_subdirectory(cave_demo)