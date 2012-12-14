cmake_minimum_required(VERSION 2.8.3)



##############################################################################
## Project
##############################################################################
project("Cave Demo" CXX)
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWERCASE)
string(REPLACE " " "_" PROJECT_NAME_LOWERCASE_UNDERSCORES ${PROJECT_NAME_LOWERCASE})
set(TARGET_NAME ${PROJECT_NAME_LOWERCASE_UNDERSCORES})



##############################################################################
## Common
##############################################################################
include(${PROJECT_SOURCE_DIR}/../cmake/common.cmake)



##############################################################################
## Definitions
##############################################################################
if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
endif()



##############################################################################
## Directories
##############################################################################
set(HPP_DIR ${PROJECT_SOURCE_DIR}/${PROJECT_NAME_LOWERCASE_UNDERSCORES})
set(CPP_DIR ${PROJECT_SOURCE_DIR}/binaries)

include_directories(${PROJECT_SOURCE_DIR})



##############################################################################
## External Libraries
##############################################################################

## Boost
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.50.0 REQUIRED date_time system chrono thread program_options filesystem serialization log_setup log)
include_directories(${Boost_INCLUDE_DIRS})

## Boost Lockfree
set(Boost_Lockfree_ROOT "Boost_Lockfree_ROOT-NOTFOUND" CACHE PATH "The root directory of Boost Lockfree.")
if(NOT Boost_Lockfree_ROOT)
	message(FATAL_ERROR "Please set Boost_Lockfree_ROOT to the root directory of Boost Lockfree. Boost Lockfree is NOT a Boost library but available through git at:\n  git://tim.klingt.org/boost_lockfree.git")
endif()
include_directories(${Boost_Lockfree_ROOT})

## Black Label
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../cmake)
find_package(BlackLabel REQUIRED "renderer 0.1" "dynamics 0.1" "thread_pool 0.1" "file_system_watcher 0.1" "file_buffer 0.1")
include_directories(${BlackLabel_INCLUDE_DIRS})

## GLM
set(GLM_ROOT "GLM_ROOT-NOTFOUND" CACHE PATH "The root directory of GLM.")
if(NOT GLM_ROOT)
	message(FATAL_ERROR "Please set GLM_ROOT to the root directory of GLM. GLM is available at:\n  http://glm.g-truc.net/download.html")
endif()
include_directories(${GLM_ROOT})

## OpenGL
find_package(OpenGL)

## SFML
set(SFML_STATIC_LIBRARIES TRUE)
find_package(SFML REQUIRED system window graphics)
include_directories(${SFML_INCLUDE_DIR})

## SFML Dependencies
if(APPLE)
	find_library(OSX_COREFOUNDATION_LIBRARY CoreFoundation)
	find_library(OSX_COCOA_LIBRARY Cocoa)
	find_library(OSX_IOKIT_LIBRARY IOKit)
	find_library(OSX_CARBON_LIBRARY Carbon)
endif()



## Black Label Dependencies

## GLEW
find_package(GLEW)
add_definitions(-DGLEW_STATIC)
include_directories(${GLEW_INCLUDE_PATH})

## ZLIB
find_package(ZLIB)
include_directories(${ZLIB_INCLUDE_DIR})

## ASSIMP
set(ASSIMP_INCLUDE_DIR "ASSIMP_INCLUDE_DIR-NOTFOUND" CACHE PATH "The include directory.")
if(NOT ASSIMP_INCLUDE_DIR)
	message(FATAL_ERROR "Please set ASSIMP_INCLUDE_DIR to the include directory of ASSIMP. ASSIMP is available at:\n  http://assimp.sourceforge.net/main_downloads.html")
endif()
include_directories(${ASSIMP_INCLUDE_DIR})

set(ASSIMP_LIBRARY_RELEASE "ASSIMP_LIBRARY_RELEASE-NOTFOUND" CACHE FILEPATH "The release version of the library.")
if(NOT ASSIMP_LIBRARY_RELEASE)
	message(FATAL_ERROR "Please set ASSIMP_LIBRARY_RELEASE to the release version of the ASSIMP library. ASSIMP is available at:\n  http://assimp.sourceforge.net/main_downloads.html")
endif()
set(ASSIMP_LIBRARY_DEBUG "ASSIMP_LIBRARY_DEBUG-NOTFOUND" CACHE FILEPATH "The debug version of the library.")
if(NOT ASSIMP_LIBRARY_DEBUG)
	message(FATAL_ERROR "Please set ASSIMP_LIBRARY_DEBUG to the debug version of the ASSIMP library. ASSIMP is available at:\n  http://assimp.sourceforge.net/main_downloads.html")
endif()
set(ASSIMP_LIBRARIES debug ${ASSIMP_LIBRARY_DEBUG} optimized ${ASSIMP_LIBRARY_RELEASE})

## FBX
set(FBX_INCLUDE_DIR "FBX_INCLUDE_DIR-NOTFOUND" CACHE PATH "The include directory.")
if(NOT FBX_INCLUDE_DIR)
	message(FATAL_ERROR "Please set FBX_INCLUDE_DIR to the include directory of FBX. FBX is available at:\n  http://usa.autodesk.com/adsk/servlet/pc/item?siteID=123112&id=10775847")
endif()
include_directories(${FBX_INCLUDE_DIR})

set(FBX_LIBRARY_RELEASE "FBX_LIBRARY_RELEASE-NOTFOUND" CACHE FILEPATH "The release version of the library.")
if(NOT FBX_LIBRARY_RELEASE)
	message(FATAL_ERROR "Please set FBX_LIBRARY_RELEASE to the release version of the FBX library. FBX is available at:\n  http://usa.autodesk.com/adsk/servlet/pc/item?siteID=123112&id=10775847")
endif()
set(FBX_LIBRARY_DEBUG "FBX_LIBRARY_DEBUG-NOTFOUND" CACHE FILEPATH "The debug version of the library.")
if(NOT FBX_LIBRARY_DEBUG)
	message(FATAL_ERROR "Please set FBX_LIBRARY_DEBUG to the debug version of the FBX library. FBX is available at:\n  http://usa.autodesk.com/adsk/servlet/pc/item?siteID=123112&id=10775847")
endif()
set(FBX_LIBRARIES debug ${FBX_LIBRARY_DEBUG} optimized ${FBX_LIBRARY_RELEASE})

## BULLET
find_package(BULLET)
set(BULLET_WORLD_IMPORTER_LIBRARY "BULLET_WORLD_IMPORTER_LIBRARY-NOTFOUND" CACHE FILEPATH "Path to the library.")
set(BULLET_WORLD_IMPORTER_LIBRARY_DEBUG "BULLET_WORLD_IMPORTER_LIBRARY_DEBUG-NOTFOUND" CACHE FILEPATH "Path to the library.")
set(BULLET_FILE_LOADER_LIBRARY "BULLET_FILE_LOADER_LIBRARY-NOTFOUND" CACHE FILEPATH "Path to the library.")
set(BULLET_FILE_LOADER_LIBRARY_DEBUG "BULLET_FILE_LOADER_LIBRARY_DEBUG-NOTFOUND" CACHE FILEPATH "Path to the library.")
list(APPEND BULLET_LIBRARIES 
	"optimized" ${BULLET_WORLD_IMPORTER_LIBRARY}
	"debug" ${BULLET_WORLD_IMPORTER_LIBRARY_DEBUG}
	"optimized" ${BULLET_FILE_LOADER_LIBRARY}
	"debug" ${BULLET_FILE_LOADER_LIBRARY_DEBUG}
	)

set(BULLET_WORLD_IMPORTER_INCLUDE_DIR "BULLET_WORLD_IMPORTER_INCLUDE_DIR-NOTFOUND" CACHE FILEPATH "Path to the include dir.")
set(BULLET_FILE_LOADER_INCLUDE_DIR "BULLET_FILE_LOADER_INCLUDE_DIR-NOTFOUND" CACHE FILEPATH "Path to the include dir.")
include_directories(${BULLET_INCLUDE_DIR} ${BULLET_WORLD_IMPORTER_INCLUDE_DIR} ${BULLET_FILE_LOADER_INCLUDE_DIR})



##############################################################################
## Search for Files
##############################################################################
set(${TARGET_NAME}_HPP_DIR ${PROJECT_SOURCE_DIR}/${PROJECT_NAME_LOWERCASE_UNDERSCORES})
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
	${BlackLabel_LIBRARIES}
	${BULLET_LIBRARIES}
	${OPENGL_gl_LIBRARY}
	
	${SFML_LIBRARIES}
	${OSX_COREFOUNDATION_LIBRARY}
	${OSX_COCOA_LIBRARY}
	${OSX_IOKIT_LIBRARY}
	${OSX_CARBON_LIBRARY}
	
	${GLEW_LIBRARY}
	${ZLIB_LIBRARY}
	${ASSIMP_LIBRARIES}
	${FBX_LIBRARIES}
	)

	

##############################################################################
## Target Properties
##############################################################################
set_target_properties_output(${TARGET_NAME} RUNTIME DEFAULT)


