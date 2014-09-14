cmake_minimum_required(VERSION 2.8)
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)
SET_PROPERTY(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER CMake)
enable_testing()



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



##############################################################################
## Dependencies
##############################################################################
add_dependencies(
	cave_demo
	renderer
	file_system_watcher
	file_buffer)
