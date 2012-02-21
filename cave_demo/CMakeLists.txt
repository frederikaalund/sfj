cmake_minimum_required(VERSION 2.8.3)



##############################################################################
## Project
##############################################################################
project("Cave Demo" CXX)
set(PROJECT_NAME cave_demo)
set(TARGET_NAME ${PROJECT_NAME})



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



##############################################################################
## External Libraries
##############################################################################

## Boost
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.46.1 REQUIRED program_options)
include_directories(${Boost_INCLUDE_DIR})

## Black Label

function(black_label_version_suffix CONFIGURATION API_VERSION VERSION_SUFFIX)
	set(VERSION_SUFFIX_LOCAL -${TOOLSET})
	if (NOT ${THREADING_${CONFIGURATION}} STREQUAL "")
		set(VERSION_SUFFIX_LOCAL ${VERSION_SUFFIX_LOCAL}-${THREADING_${CONFIGURATION}})
	endif()
	if (NOT ${ABI_${CONFIGURATION}} STREQUAL "")
		set(VERSION_SUFFIX_LOCAL ${VERSION_SUFFIX_LOCAL}-${ABI_${CONFIGURATION}})
	endif()
	if (NOT ${API_VERSION} STREQUAL "")
		string(REPLACE "." "_" API_VERSION_FORMATTED ${API_VERSION})
		set(VERSION_SUFFIX_LOCAL ${VERSION_SUFFIX_LOCAL}-${API_VERSION_FORMATTED})
	endif()
	set(${VERSION_SUFFIX} ${VERSION_SUFFIX_LOCAL} PARENT_SCOPE)
endfunction()



set(BLACK_LABEL_COMPONENTS "log 0.1" "shared_library 0.1")
set(BLACK_LABEL_ROOT_DOCSTRING "Black Label root directory")
set(BLACK_LABEL_ROOT "BLACK_LABEL_ROOT-NOTFOUND" CACHE PATH ${BLACK_LABEL_ROOT_DOCSTRING})
if(NOT BLACK_LABEL_ROOT)
	if(IS_DIRECTORY ${CMAKE_SOURCE_DIR}/black_label)
		set(BLACK_LABEL_ROOT ${CMAKE_SOURCE_DIR}/black_label CACHE PATH ${BLACK_LABEL_ROOT_DOCSTRING} FORCE)
	else()
		message(FATAL_ERROR "Please set BLACK_LABEL_ROOT to the root directory of Black Label.")
	endif()
endif()
set(BLACK_LABEL_LIBRARY_DIR ${BLACK_LABEL_ROOT}/stage/libraries CACHE PATH "Black Label library directory.")
set(BLACK_LABEL_INCLUDE_DIR ${BLACK_LABEL_ROOT} CACHE PATH "Black Label include directory.")

foreach(_COMPONENT ${BLACK_LABEL_COMPONENTS})
	string(REGEX MATCH "^[^ ]*" _COMPONENT_NAME ${_COMPONENT})
	string(REGEX MATCH " .*$" _COMPONENT_VERSION ${_COMPONENT})
	string(STRIP ${_COMPONENT_NAME} _COMPONENT_NAME)
	
	if (NOT _COMPONENT_VERSION)
		message(FATAL_ERROR "Black Label component ${_COMPONENT_NAME} is missing its version number! The syntax is:\n  <component> <version> [<component> <version> ...]")
	endif()
	
	string(TOUPPER ${_COMPONENT_NAME} _COMPONENT_NAME_CAPITALIZED)
	string(STRIP ${_COMPONENT_VERSION} _COMPONENT_VERSION)

	if(WIN32)
		set(PREFIX "")
		set(EXTENSION ".lib")
	elseif(UNIX)
		set(PREFIX "lib")
		set(EXTENSION ".a")
	else()
		message(FATAL_ERROR "Unknown target platform! Known platforms:\n  Windows, Unix, and Unix like")
	endif()
	
	black_label_version_suffix(RELEASE ${_COMPONENT_VERSION} SUFFIX)
	set(BLACK_LABEL_${_COMPONENT_NAME_CAPITALIZED}_RELEASE ${BLACK_LABEL_LIBRARY_DIR}/${PREFIX}black_label_${_COMPONENT_NAME}${SUFFIX}${EXTENSION} CACHE PATH "Debug version of tge ${_COMPONENT_NAME} library.")
	black_label_version_suffix(DEBUG ${_COMPONENT_VERSION} SUFFIX)
	set(BLACK_LABEL_${_COMPONENT_NAME_CAPITALIZED}_DEBUG ${BLACK_LABEL_LIBRARY_DIR}/${PREFIX}black_label_${_COMPONENT_NAME}${SUFFIX}${EXTENSION} CACHE PATH "Release version of the ${_COMPONENT_NAME} library.")
	
	list(
		APPEND BLACK_LABEL_LIBRARIES
		optimized	${BLACK_LABEL_${_COMPONENT_NAME_CAPITALIZED}_RELEASE}
		debug		${BLACK_LABEL_${_COMPONENT_NAME_CAPITALIZED}_DEBUG}
		)
endforeach()

include_directories(${BLACK_LABEL_INCLUDE_DIR})



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
## Linking
##############################################################################
target_link_libraries(
	${TARGET_NAME}
	${Boost_LIBRARIES}
	${BLACK_LABEL_LIBRARIES}
	)



##############################################################################
## Target Properties
##############################################################################
set_target_properties_output(${TARGET_NAME} RUNTIME)



