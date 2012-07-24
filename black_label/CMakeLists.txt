cmake_minimum_required(VERSION 2.8)



##############################################################################
## Project
##############################################################################
project("Black Label" CXX)
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWERCASE)
string(REPLACE " " "_" PROJECT_NAME_LOWERCASE_UNDERSCORES ${PROJECT_NAME_LOWERCASE})



##############################################################################
## Common
##############################################################################
include(${PROJECT_SOURCE_DIR}/../cmake/common.cmake)



##############################################################################
## Directories
##############################################################################
set(HPP_DIR ${PROJECT_SOURCE_DIR}/${PROJECT_NAME_LOWERCASE_UNDERSCORES})
set(CPP_DIR ${PROJECT_SOURCE_DIR}/libraries)

include_directories(${PROJECT_SOURCE_DIR})



##############################################################################
## Internal Libraries
##############################################################################

## Black Label
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../../sfj/cmake)
find_package(BlackLabel REQUIRED "file_buffer 0.1")



##############################################################################
## External Libraries
##############################################################################

## Boost
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.49.0 REQUIRED date_time thread unit_test_framework)
include_directories(${Boost_INCLUDE_DIRS})

## Boost Lockfree
set(Boost_Lockfree_ROOT "Boost_Lockfree_ROOT-NOTFOUND" CACHE PATH "The root directory of Boost Lockfree.")
if(NOT Boost_Lockfree_ROOT)
	message(FATAL_ERROR "Please set Boost_Lockfree_ROOT to the root directory of Boost Lockfree. Boost Lockfree is NOT a Boost library but available through git at:\n  git://tim.klingt.org/boost_lockfree.git")
endif()
include_directories(${Boost_Lockfree_ROOT})

## GLM
set(GLM_ROOT "GLM_ROOT-NOTFOUND" CACHE PATH "The root directory of GLM.")
if(NOT GLM_ROOT)
	message(FATAL_ERROR "Please set GLM_ROOT to the root directory of GLM. GLM is available at:\n  http://glm.g-truc.net/download.html")
endif()
include_directories(${GLM_ROOT})

## GLEW
find_package(GLEW)
include_directories(${GLEW_INCLUDE_PATH})

## GLUT
find_package(OpenGL)

## ASSIMP
set(ASSIMP_ROOT "ASSIMP_ROOT-NOTFOUND" CACHE PATH "The root directory of ASSIMP.")
if(NOT ASSIMP_ROOT)
	message(FATAL_ERROR "Please set ASSIMP_ROOT to the root directory of ASSIMP. ASSIMP is available at:\n  http://assimp.sourceforge.net/main_downloads.html")
endif()
include_directories(${ASSIMP_ROOT}/include)
set(ASSIMP_LIBRARIES debug ${ASSIMP_ROOT}/lib/debug/assimp.lib optimized ${ASSIMP_ROOT}/lib/release/assimp.lib)



##############################################################################
## Main HPPs
##############################################################################
file(GLOB MAIN_HPPS ${HPP_DIR}/*.hpp)



##############################################################################
## Main HPPs
##############################################################################
set(BlackLabel_RUNTIME_STAGE_DIR DEFAULT CACHE PATH "Where to put compiled runtime targets (.exe, .dll, .so)." )



##############################################################################
## A library for each main HPP
##############################################################################
foreach(MAIN_HPP ${MAIN_HPPS})
	get_filename_component(TARGET_NAME ${MAIN_HPP} NAME_WE)
	
	

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

	directory_source_group(HPP\\${TARGET_NAME} ${${TARGET_NAME}_HPP_DIR} hpp)
	source_group(HPP FILES ${MAIN_HPP})
	directory_source_group(CPP\\${TARGET_NAME} ${${TARGET_NAME}_CPP_DIR}/${TARGET_NAME} cpp)
	source_group(CPP FILES ${${TARGET_NAME}_CPP_DIR}/${TARGET_NAME}.cpp)
	source_group(CPP FILES ${${TARGET_NAME}_TEST_CPPS})

	
		
	##############################################################################
	## Header-only
	##############################################################################
	if(NOT IS_DIRECTORY ${${TARGET_NAME}_CPP_DIR})	
		set(TARGET_TARGET_TYPE HEADER_ONLY)
		#add_custom_target(${TARGET_NAME} SOURCES ${${TARGET_NAME}_HPPS})
		add_library(${TARGET_NAME} STATIC ${${TARGET_NAME}_HPPS})
		set_target_properties(${TARGET_NAME} PROPERTIES LINKER_LANGUAGE CXX)
		
		
		
		
	##############################################################################
	## Binary
	##############################################################################
	else()
	
	
	
		##############################################################################
		## Type
		##############################################################################
		if(${TARGET_NAME} STREQUAL thread_pool OR ${TARGET_NAME} STREQUAL renderer)
			set(TARGET_LIBRARY_TYPE SHARED)
			set(TARGET_TARGET_TYPE LIBRARY)
		else()
			string(TOUPPER ${TARGET_NAME} TARGET_NAME_CAPITALIZED)
			set(BlackLabel_BUILD_STATIC_${TARGET_NAME_CAPITALIZED} CACHE BOOL "Build ${TARGET_NAME} as a static library.")
			if(BlackLabel_BUILD_STATIC_${TARGET_NAME_CAPITALIZED})
				set(TARGET_LIBRARY_TYPE STATIC)
				set(TARGET_TARGET_TYPE ARCHIVE)
			else()
				set(TARGET_LIBRARY_TYPE SHARED)
				set(TARGET_TARGET_TYPE LIBRARY)
			endif()
		endif()
	
	
	
		##############################################################################
		## Binary
		##############################################################################	
		add_library(${TARGET_NAME} ${TARGET_LIBRARY_TYPE} ${${TARGET_NAME}_HPPS} ${${TARGET_NAME}_CPPS})

		

		##############################################################################
		## Linking
		##############################################################################
		if(${TARGET_NAME} STREQUAL thread_pool)
			target_link_libraries(
				${TARGET_NAME}
				${Boost_DATE_TIME_LIBRARIES}
				${Boost_THREAD_LIBRARIES}
				)
		elseif(${TARGET_NAME} STREQUAL renderer)
			target_link_libraries(
				${TARGET_NAME}
				${BlackLabel_FILE_BUFFER_LIBRARIES}
				${ASSIMP_LIBRARIES}
				${GLEW_LIBRARY}
				${OPENGL_gl_LIBRARY}
				)
		endif()	
		


		##############################################################################
		## Properties
		##############################################################################
		set_target_properties(${TARGET_NAME} PROPERTIES
			SOVERSION ${API_VERSION}
			)
	endif()
	


	##############################################################################
	## Properties
	##############################################################################
	set_target_properties_output(${TARGET_NAME} ${TARGET_TARGET_TYPE} ${BlackLabel_RUNTIME_STAGE_DIR})
	
	
	
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
			set_target_properties_output(${TEST_NAME} TEST ${BlackLabel_RUNTIME_STAGE_DIR})
		endforeach()
	endif()
endforeach()



##############################################################################
## Inter-dependencies
##############################################################################
add_dependencies(renderer file_buffer)