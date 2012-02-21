cmake_minimum_required(VERSION 2.8)



##############################################################################
## Project
##############################################################################
project("Black Label" CXX)
set(PROJECT_NAME black_label)



##############################################################################
## Common
##############################################################################
include(${PROJECT_SOURCE_DIR}/../cmake/common.cmake)



##############################################################################
## Directories
##############################################################################
set(HPP_DIR ${PROJECT_SOURCE_DIR}/${PROJECT_NAME})
set(CPP_DIR ${PROJECT_SOURCE_DIR}/libraries)

include_directories(${PROJECT_SOURCE_DIR})



##############################################################################
## External Libraries
##############################################################################

## Boost
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.46.1 REQUIRED COMPONENTS date_time thread)
include_directories(${Boost_INCLUDE_DIR})

## Boost Lockfree
set(Boost_Lockfree_ROOT "Boost_Lockfree_ROOT-NOTFOUND" CACHE PATH "The root directory of Boost Lockfree.")
if(NOT Boost_Lockfree_ROOT)
	message(FATAL_ERROR "Please set Boost_Lockfree_ROOT to the root directory of Boost Lockfree. Boost Lockfree is NOT a Boost library but available through git at:\n  git://tim.klingt.org/boost_lockfree.git")
endif()
include_directories(${Boost_Lockfree_ROOT})



##############################################################################
## Main HPPs
##############################################################################
file(GLOB MAIN_HPPS ${HPP_DIR}/*.hpp)



##############################################################################
## A library for each main HPP
##############################################################################
foreach(MAIN_HPP ${MAIN_HPPS})
	get_filename_component(TARGET_NAME ${MAIN_HPP} NAME_WE)
	
	
	
	##############################################################################
	## Type
	##############################################################################
	string(TOUPPER ${TARGET_NAME} TARGET_NAME_CAPITALIZED)
	set(BLACK_LABEL_BUILD_STATIC_${TARGET_NAME_CAPITALIZED} CACHE BOOL "Build ${TARGET_NAME} as a static library.")
	if(BLACK_LABEL_BUILD_STATIC_${TARGET_NAME_CAPITALIZED})
		set(TARGET_LIBRARY_TYPE STATIC)
		set(TARGET_TARGET_TYPE ARCHIVE)
	else()
		set(TARGET_LIBRARY_TYPE MODULE)
		set(TARGET_TARGET_TYPE LIBRARY)
	endif()
	
	

	##############################################################################
	## API Version
	##############################################################################	
	file(STRINGS ${MAIN_HPP} VERSION_STRING LIMIT_COUNT 1)
	string(SUBSTRING ${VERSION_STRING} 11 -1 API_VERSION)
	string(REPLACE "." "_" API_VERSION_NAME ${API_VERSION})



	##############################################################################
	## Search for Files
	##############################################################################
	set(${TARGET_NAME}_HPP_DIR ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}/${TARGET_NAME})
	set(${TARGET_NAME}_CPP_DIR ${PROJECT_SOURCE_DIR}/libraries/${TARGET_NAME}/source)

	file(GLOB_RECURSE ${TARGET_NAME}_HPPS ${${TARGET_NAME}_HPP_DIR}/*.hpp)
	file(GLOB_RECURSE ${TARGET_NAME}_CPPS ${${TARGET_NAME}_CPP_DIR}/*.cpp)
	list(APPEND ${TARGET_NAME}_HPPS ${MAIN_HPP})

	directory_source_group(HPP\\${TARGET_NAME} ${${TARGET_NAME}_HPP_DIR} hpp)
	directory_source_group(CPP\\${TARGET_NAME} ${${TARGET_NAME}_CPP_DIR}/${TARGET_NAME} cpp)
	source_group(HPP FILES ${MAIN_HPP})
	source_group(CPP FILES ${${TARGET_NAME}_CPP_DIR}/${TARGET_NAME}.cpp)

	

	##############################################################################
	## Binaries
	##############################################################################	
	add_library(${TARGET_NAME} ${TARGET_LIBRARY_TYPE} ${${TARGET_NAME}_HPPS} ${${TARGET_NAME}_CPPS})
	
	
	
	##############################################################################
	## Linking
	##############################################################################
	if(${TARGET_NAME} STREQUAL thread_pool)
		target_link_libraries(
			${TARGET_NAME}
			${Boost_LIBRARIES}
			)
	endif()

	

	##############################################################################
	## Target Properties
	##############################################################################
	set_target_properties_output(${TARGET_NAME} ${TARGET_TARGET_TYPE})
	
	set_target_properties(${TARGET_NAME} PROPERTIES
		SOVERSION ${API_VERSION}
	)
endforeach()