cmake_minimum_required(VERSION 2.8)



##############################################################################
## Project
##############################################################################
project("Cave Demo" CXX)
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWERCASE)
string(REPLACE " " "_" PROJECT_NAME_LOWERCASE_UNDERSCORES ${PROJECT_NAME_LOWERCASE})
set(TARGET_NAME ${PROJECT_NAME_LOWERCASE_UNDERSCORES})



##############################################################################
## Files & Directories
##############################################################################
find_path(SFJ_CMAKE_DIR common.cmake HINTS ${SFJ_SOURCE_DIR}/cmake)
find_file(SFJ_COMMON common.cmake HINTS ${SFJ_CMAKE_DIR})

include(${SFJ_COMMON})
list(APPEND CMAKE_MODULE_PATH ${SFJ_CMAKE_DIR})

set(HPP_DIR ${PROJECT_SOURCE_DIR}/${PROJECT_NAME_LOWERCASE_UNDERSCORES})
set(CPP_DIR ${PROJECT_SOURCE_DIR}/binaries)

include_directories(${PROJECT_SOURCE_DIR})

set(CaveDemo_RUNTIME_STAGE_DIR ${PROJECT_SOURCE_DIR}/stage/binaries
	CACHE PATH "Where to put compiled runtimes (.exe, .dll, .so).")



##############################################################################
## Definitions
##############################################################################
if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
endif()



##############################################################################
## Libraries
##############################################################################

# Black Label
set(BlackLabel_USE_STATIC_LIBS ON)
find_package(BlackLabel 0.1 QUIET REQUIRED renderer dynamics thread_pool file_system_watcher file_buffer)
include_directories(${BlackLabel_INCLUDE_DIRS})

# Boost
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.50.0 QUIET REQUIRED program_options log_setup log filesystem system date_time thread)
include_directories(${Boost_INCLUDE_DIRS})

# SFML
set(SFML_STATIC_LIBRARIES TRUE)
find_package(SFML REQUIRED system window)
include_directories(${SFML_INCLUDE_DIR})

# OSX Libraries (SFML Dependencies)
if(APPLE)
	find_library(OSX_COREFOUNDATION_LIBRARY CoreFoundation)
	find_library(OSX_COCOA_LIBRARY Cocoa)
	find_library(OSX_IOKIT_LIBRARY IOKit)
	find_library(OSX_CARBON_LIBRARY Carbon)
endif()



##############################################################################
## Search for Files
##############################################################################
set(${TARGET_NAME}_HPP_DIR ${PROJECT_SOURCE_DIR}/${PROJECT_NAME_LOWERCASE_UNDERSCORES})
set(${TARGET_NAME}_CPP_DIR ${PROJECT_SOURCE_DIR}/binaries/${TARGET_NAME}/source)

file(GLOB_RECURSE ${TARGET_NAME}_HPPS ${${TARGET_NAME}_HPP_DIR}/*.hpp)
file(GLOB_RECURSE ${TARGET_NAME}_CPPS ${${TARGET_NAME}_CPP_DIR}/*.cpp)
directory_structured_source_groups(${${TARGET_NAME}_HPP_DIR} hpp HPP\\${TARGET_NAME})
directory_structured_source_groups(${${TARGET_NAME}_CPP_DIR} cpp CPP\\${TARGET_NAME})
source_group(CPP FILES ${${TARGET_NAME}_CPP_DIR}/main.cpp)
source_group(CPP FILES ${${TARGET_NAME}_CPP_DIR}/runtime_interface.cpp)



##############################################################################
## Binaries
##############################################################################
add_executable(${TARGET_NAME} ${${TARGET_NAME}_HPPS} ${${TARGET_NAME}_CPPS})



##############################################################################
## Linking
##############################################################################
target_link_libraries(
	${TARGET_NAME}
	${Boost_LIBRARIES}
	${BlackLabel_LIBRARIES}
	
	${SFML_LIBRARIES}
	${OSX_COREFOUNDATION_LIBRARY}
	${OSX_COCOA_LIBRARY}
	${OSX_IOKIT_LIBRARY}
	${OSX_CARBON_LIBRARY})

	

##############################################################################
## Target Properties
##############################################################################
set_target_output_properties(${TARGET_NAME} RUNTIME ${CaveDemo_RUNTIME_STAGE_DIR})
