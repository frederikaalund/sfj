cmake_minimum_required(VERSION 2.8)



##############################################################################
## Project
##############################################################################
project("Black Label" CXX)
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWERCASE)
string(REPLACE " " "_" PROJECT_NAME_LOWERCASE_UNDERSCORES ${PROJECT_NAME_LOWERCASE})



##############################################################################
## Files & Directories
##############################################################################
find_path(SFJ_CMAKE_DIR common.cmake HINTS ${SFJ_SOURCE_DIR}/cmake)
find_file(SFJ_COMMON common.cmake HINTS ${SFJ_CMAKE_DIR})

include(${SFJ_COMMON})
list(APPEND CMAKE_MODULE_PATH ${SFJ_CMAKE_DIR})

set(HPP_DIR ${PROJECT_SOURCE_DIR}/${PROJECT_NAME_LOWERCASE_UNDERSCORES})
set(CPP_DIR ${PROJECT_SOURCE_DIR}/libraries)

include_directories(${PROJECT_SOURCE_DIR})

set(BlackLabel_RUNTIME_STAGE_DIR ${PROJECT_SOURCE_DIR}/stage/binaries
	CACHE PATH "Where to put compiled runtimes (.exe, .dll, .so).")
set(BlackLabel_ARCHIVE_STAGE_DIR ${PROJECT_SOURCE_DIR}/stage/libraries
	CACHE PATH "Where to put compiled archives or libraries (.lib, .a).")



##############################################################################
## Definitions
##############################################################################
if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
endif()



##############################################################################
## Libraries
#############################################################################
# Boost
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.50.0 QUIET REQUIRED unit_test_framework)

include(${PROJECT_SOURCE_DIR}/cmake/dependencies_common.cmake)
include_directories(${BlackLabel_DEPENDENCIES_COMMON_INCLUDE_DIRS})



##############################################################################
## Main HPPs
##############################################################################
file(GLOB MAIN_HPPS ${HPP_DIR}/*.hpp)




##############################################################################
## A library for each main HPP
##############################################################################
foreach(MAIN_HPP ${MAIN_HPPS})
	get_filename_component(TARGET_NAME ${MAIN_HPP} NAME_WE)
	string(TOUPPER ${TARGET_NAME} TARGET_NAME_CAPITALIZED)
	
	

	##############################################################################
	## API Version
	##############################################################################	
	file(STRINGS ${MAIN_HPP} VERSION_STRING LIMIT_COUNT 1)
	string(SUBSTRING ${VERSION_STRING} 11 -1 API_VERSION)
	string(REPLACE "." "_" API_VERSION_NAME ${API_VERSION})



	##############################################################################
	## Search for Files
	##############################################################################
	set(${TARGET_NAME}_HPP_DIR ${PROJECT_SOURCE_DIR}/${PROJECT_NAME_LOWERCASE_UNDERSCORES}/${TARGET_NAME})
	set(${TARGET_NAME}_CPP_DIR ${PROJECT_SOURCE_DIR}/libraries/${TARGET_NAME}/source)
	set(${TARGET_NAME}_TEST_DIR ${PROJECT_SOURCE_DIR}/libraries/${TARGET_NAME}/test)
	
	

	file(GLOB_RECURSE ${TARGET_NAME}_HPPS ${${TARGET_NAME}_HPP_DIR}/*.hpp)
	file(GLOB_RECURSE ${TARGET_NAME}_CPPS ${${TARGET_NAME}_CPP_DIR}/*.cpp)
	file(GLOB_RECURSE ${TARGET_NAME}_TEST_CPPS ${${TARGET_NAME}_TEST_DIR}/*.cpp)
	list(APPEND ${TARGET_NAME}_HPPS ${MAIN_HPP})

	directory_structured_source_groups(${${TARGET_NAME}_HPP_DIR} hpp HPP\\${TARGET_NAME})
	source_group(HPP FILES ${MAIN_HPP})
	directory_structured_source_groups(${${TARGET_NAME}_CPP_DIR}/${TARGET_NAME} cpp CPP\\${TARGET_NAME})
	source_group(CPP FILES ${${TARGET_NAME}_CPP_DIR}/${TARGET_NAME}.cpp)
	source_group(CPP FILES ${${TARGET_NAME}_TEST_CPPS})

	
		
	##############################################################################
	## Header-only
	##############################################################################
	if(NOT IS_DIRECTORY ${${TARGET_NAME}_CPP_DIR})	
		set(TARGET_TARGET_TYPE HEADER_ONLY)
		add_library(${TARGET_NAME} STATIC ${${TARGET_NAME}_HPPS})
		set_target_properties(${TARGET_NAME} PROPERTIES LINKER_LANGUAGE CXX)
		
		
		
		
	##############################################################################
	## Binary
	##############################################################################
	else()
	
	
	
		##############################################################################
		## Type
		##############################################################################
		option(BlackLabel_BUILD_STATIC_${TARGET_NAME_CAPITALIZED}
			"Build ${TARGET_NAME} as a static library.")
		if(BlackLabel_BUILD_STATIC_${TARGET_NAME_CAPITALIZED})
			set(TARGET_LIBRARY_TYPE STATIC)
			set(TARGET_TARGET_TYPE ARCHIVE)
		else()
			set(TARGET_LIBRARY_TYPE SHARED)
			set(TARGET_TARGET_TYPE LIBRARY)
		endif()
	
	
	
		##############################################################################
		## Binary
		##############################################################################	
		add_library(${TARGET_NAME} ${TARGET_LIBRARY_TYPE} ${${TARGET_NAME}_HPPS} ${${TARGET_NAME}_CPPS})

		

		##############################################################################
		## Dependencies
		##############################################################################
		include(
			${PROJECT_SOURCE_DIR}/cmake/dependencies_${TARGET_NAME}.cmake
			OPTIONAL RESULT_VARIABLE DEPENDENCIES_FILE)

		if(DEPENDENCIES_FILE)
			include_directories(
				${BlackLabel_DEPENDENCIES_${TARGET_NAME_CAPITALIZED}_INCLUDE_DIRS})
			target_link_libraries(
				${TARGET_NAME}
				${BlackLabel_DEPENDENCIES_${TARGET_NAME_CAPITALIZED}_LIBRARIES})
		endif()
		


		##############################################################################
		## Properties
		##############################################################################
		set_target_properties(${TARGET_NAME} PROPERTIES
			SOVERSION ${API_VERSION})
	endif()
	


	##############################################################################
	## Properties
	##############################################################################
	set_target_output_properties(
		${TARGET_NAME}
		${TARGET_TARGET_TYPE}
		${BlackLabel_RUNTIME_STAGE_DIR}
		${BlackLabel_ARCHIVE_STAGE_DIR})
	
	
	
	##############################################################################
	## Tests
	##############################################################################
	set(TEST_LIBRARIES ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
	if(NOT ${TARGET_TARGET_TYPE} STREQUAL HEADER_ONLY)
		list(APPEND TEST_LIBRARIES ${TARGET_NAME})
	endif()
	
	if(IS_DIRECTORY ${${TARGET_NAME}_TEST_DIR})
		foreach(TEST_CPP ${${TARGET_NAME}_TEST_CPPS})
			get_filename_component(TEST_NAME ${TEST_CPP} NAME_WE)
			set(TEST_NAME ${TARGET_NAME}_${TEST_NAME})
			
			add_executable(${TEST_NAME} ${TEST_CPP})
			target_link_libraries(${TEST_NAME} ${TEST_LIBRARIES})
			set_target_output_properties(${TEST_NAME} TEST ${BlackLabel_RUNTIME_STAGE_DIR})
		endforeach()
	endif()
endforeach()



##############################################################################
## Inter-dependencies
##############################################################################
add_dependencies(renderer file_buffer)
