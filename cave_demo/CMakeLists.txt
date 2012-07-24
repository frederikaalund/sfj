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
find_package(Boost 1.46.1 REQUIRED date_time system chrono thread program_options)
include_directories(${Boost_INCLUDE_DIRS})

## Boost Lockfree
set(Boost_Lockfree_ROOT "Boost_Lockfree_ROOT-NOTFOUND" CACHE PATH "The root directory of Boost Lockfree.")
if(NOT Boost_Lockfree_ROOT)
	message(FATAL_ERROR "Please set Boost_Lockfree_ROOT to the root directory of Boost Lockfree. Boost Lockfree is NOT a Boost library but available through git at:\n  git://tim.klingt.org/boost_lockfree.git")
endif()
include_directories(${Boost_Lockfree_ROOT})

## Black Label
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../cmake)
find_package(BlackLabel REQUIRED "log 0.1" "thread_pool 0.1")
include_directories(${BlackLabel_INCLUDE_DIRS})

## GLM
set(GLM_ROOT "GLM_ROOT-NOTFOUND" CACHE PATH "The root directory of GLM.")
if(NOT GLM_ROOT)
	message(FATAL_ERROR "Please set GLM_ROOT to the root directory of GLM. GLM is available at:\n  http://glm.g-truc.net/download.html")
endif()
include_directories(${GLM_ROOT})



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
	)



##############################################################################
## Target Properties
##############################################################################
set_target_properties_output(${TARGET_NAME} RUNTIME DEFAULT)


