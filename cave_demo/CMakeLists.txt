cmake_minimum_required(VERSION 2.8.3)



##############################################################################
## Project
##############################################################################
project("Cave Demo" CXX)
set(PROJECT_NAME cave_demo)



##############################################################################
## Common
##############################################################################
include(${PROJECT_SOURCE_DIR}/../cmake/common.cmake)



##############################################################################
## Directories
##############################################################################
set(HPP_DIR ${PROJECT_SOURCE_DIR}/${PROJECT_NAME})
set(CPP_DIR ${PROJECT_SOURCE_DIR}/binaries)



include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/../black_label)

set(BLACK_LABEL_LIBRARY_DIR ${PROJECT_SOURCE_DIR}/../black_label/stage/libraries)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.46.1 REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIR})



set(TARGET_NAME ${PROJECT_NAME})



##############################################################################
## Search for Files
##############################################################################
set(${TARGET_NAME}_HPP_DIR ${PROJECT_SOURCE_DIR}/${PROJECT_NAME})
set(${TARGET_NAME}_CPP_DIR ${PROJECT_SOURCE_DIR}/binaries/${TARGET_NAME}/source)

file(GLOB_RECURSE ${TARGET_NAME}_HPPS ${${TARGET_NAME}_HPP_DIR}/*.hpp)
file(GLOB_RECURSE ${TARGET_NAME}_CPPS ${${TARGET_NAME}_CPP_DIR}/*.cpp)

directory_source_group(HPP\\${TARGET_NAME} ${${TARGET_NAME}_HPP_DIR} hpp)
directory_source_group(CPP\\${TARGET_NAME} ${${TARGET_NAME}_CPP_DIR} cpp)
source_group(CPP FILES ${${TARGET_NAME}_CPP_DIR}/main.cpp)
source_group(CPP FILES ${${TARGET_NAME}_CPP_DIR}/runtime_interface.cpp)



##############################################################################
## Binaries
##############################################################################
add_executable(${TARGET_NAME} ${${TARGET_NAME}_HPPS} ${${TARGET_NAME}_CPPS})



##############################################################################
## External Libraries
##############################################################################
target_link_libraries(
	${TARGET_NAME}
	debug ${BLACK_LABEL_LIBRARY_DIR}/black_label_shared_library-vc100-mt-gd-0_1.lib
	debug ${BLACK_LABEL_LIBRARY_DIR}/black_label_log-vc100-mt-gd-0_1.lib
	optimized ${BLACK_LABEL_LIBRARY_DIR}/black_label_shared_library-vc100-mt-0_1.lib
	optimized ${BLACK_LABEL_LIBRARY_DIR}/black_label_log-vc100-mt-0_1.lib
	${Boost_LIBRARIES}
	)



##############################################################################
## Target Properties
##############################################################################
set_target_properties_output(${TARGET_NAME} RUNTIME)



