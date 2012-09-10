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
## Work-arounds
##############################################################################
# Multiple find_package invocations does not properly aggregate the different components
# AFTER the initial configuration. Fortunately, this won't be an issue for end users
# as the initial configuration run should be enough. A developer, however, may want
# to change the component lists and find that it doesn't work.
# For now the boost components of both black_label and cave_demo are manually aggregated
# here instead.
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.50.0 REQUIRED date_time system chrono thread unit_test_framework program_options filesystem log_setup log)



##############################################################################
## Common
##############################################################################
add_subdirectory(black_label)
add_subdirectory(cave_demo)