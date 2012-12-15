##############################################################################
## Definitions
##############################################################################
if(WIN32)
	add_definitions("-DWINDOWS")
elseif(UNIX)
	add_definitions("-DUNIX")
endif()
if(APPLE)
	add_definitions("-DAPPLE")
endif()

if(MSVC)
	add_definitions("-DMSVC")
	add_definitions("-DMSVC_VERSION=${MSVC_VERSION}")
elseif(XCODE_VERSION)
	add_definitions("-DXCODE")
endif()



##############################################################################
## Add Build Types & Configurations
##############################################################################
function(add_build_type_and_configuration NAME BASE CXX_FLAGS)
	string(TOUPPER ${NAME} NAME_CAPITALIZED)
	string(TOUPPER ${BASE} BASE_CAPITALIZED)

	set(CMAKE_CXX_FLAGS_${NAME_CAPITALIZED}
	    "${CMAKE_CXX_FLAGS_${BASE_CAPITALIZED}} ${CXX_FLAGS}"
	    CACHE STRING "Flags used by the C++ compiler during ${NAME} builds."
	    FORCE)
	set(CMAKE_EXE_LINKER_FLAGS_${NAME_CAPITALIZED}
	    ${CMAKE_EXE_LINKER_FLAGS_${BASE_CAPITALIZED}}
	    CACHE STRING "Flags used for linking binaries during ${NAME} builds."
	    FORCE)
	set(CMAKE_MODULE_LINKER_FLAGS_${NAME_CAPITALIZED}
	    ${CMAKE_MODULE_LINKER_FLAGS_${BASE_CAPITALIZED}}
	    CACHE STRING "Flags used by the modules linker during ${NAME} builds."
	    FORCE)
	set(CMAKE_SHARED_LINKER_FLAGS_${NAME_CAPITALIZED}
	    ${CMAKE_SHARED_LINKER_FLAGS_${BASE_CAPITALIZED}}
	    CACHE STRING "Flags used by the shared libraries linker during ${NAME} builds."
	    FORCE)
	mark_as_advanced(
	    CMAKE_CXX_FLAGS_${NAME_CAPITALIZED}
	    CMAKE_C_FLAGS_${NAME_CAPITALIZED}
	    CMAKE_EXE_LINKER_FLAGS_${NAME_CAPITALIZED}
	    CMAKE_SHARED_LINKER_FLAGS_${NAME_CAPITALIZED})

	if(CMAKE_CONFIGURATION_TYPES)
		list(APPEND CMAKE_CONFIGURATION_TYPES ${NAME})
		list(REMOVE_DUPLICATES CMAKE_CONFIGURATION_TYPES)
		set(CMAKE_CONFIGURATION_TYPES ${CMAKE_CONFIGURATION_TYPES}
	   		CACHE STRING "" FORCE)
	endif()
endfunction()

add_build_type_and_configuration(
	"ReleaseWithDeveloperTools"
	"Release"
	"/D DEVELOPER_TOOLS")
add_build_type_and_configuration(
	"MinSizeRelWithDeveloperTools"
	"MinSizeRel"
	"/D DEVELOPER_TOOLS")



##############################################################################
## Identify Configurations
##############################################################################
foreach(CONFIGURATION ${CMAKE_CONFIGURATION_TYPES})
	if(
		${CONFIGURATION} STREQUAL Debug OR
		${CONFIGURATION} STREQUAL Release OR
		${CONFIGURATION} STREQUAL MinSizeRel OR
		${CONFIGURATION} STREQUAL RelWithDebInfo OR
		${CONFIGURATION} STREQUAL ReleaseWithDeveloperTools OR
		${CONFIGURATION} STREQUAL MinSizeRelWithDeveloperTools)
		string(TOUPPER ${CONFIGURATION} CONFIGURATION_CAPITALIZED)	
		list(APPEND CONFIGURATIONS ${CONFIGURATION_CAPITALIZED})
	else()
		message(FATAL_ERROR "Unsupported configuration type: ${CONFIGURATION}")
	endif()
endforeach()



##############################################################################
## Identify Toolset
##############################################################################

# Visual Studio
if(MSVC)
	math(EXPR VC_VERSION ${MSVC_VERSION}/10-60)
	set(TOOLSET vc${VC_VERSION})
	
	# Reference: http://msdn.microsoft.com/en-us/library/2kzt1wy3(v=vs.71).aspx
	foreach(CONFIGURATION ${CONFIGURATIONS})
		if(${CMAKE_CXX_FLAGS_${CONFIGURATION}} MATCHES "/MDd")
			set(THREADING_${CONFIGURATION} mt)
			set(ABI_${CONFIGURATION} ${ABI_${CONFIGURATION}}g-)
		elseif(${CMAKE_CXX_FLAGS_${CONFIGURATION}} MATCHES "/MD")
			set(THREADING_${CONFIGURATION} mt)
		elseif(${CMAKE_CXX_FLAGS_${CONFIGURATION}} MATCHES "/MLd")
			set(ABI_${CONFIGURATION} ${ABI_${CONFIGURATION}}sg-)
		elseif(${CMAKE_CXX_FLAGS_${CONFIGURATION}} MATCHES "/ML")
			set(ABI_${CONFIGURATION} ${ABI_${CONFIGURATION}}s-)
		elseif(${CMAKE_CXX_FLAGS_${CONFIGURATION}} MATCHES "/MTd")
			set(THREADING_${CONFIGURATION} mt)
			set(ABI_${CONFIGURATION} ${ABI_${CONFIGURATION}}sg-)
		elseif(${CMAKE_CXX_FLAGS_${CONFIGURATION}} MATCHES "/MT")
			set(THREADING_${CONFIGURATION} mt)
			set(ABI_${CONFIGURATION} ${ABI_${CONFIGURATION}}s-)
		endif()
	endforeach()


# g++
elseif (CMAKE_COMPILER_IS_GNUCXX)
	# Identify compiler specifics
	execute_process(COMMAND ${CMAKE_CXX_COMPILER} "-dumpversion" OUTPUT_VARIABLE GNUCXX_VERSION)
	execute_process(COMMAND ${CMAKE_CXX_COMPILER} "-dumpmachine" OUTPUT_VARIABLE GNUCXX_TARGET_MACHINE)
	string(STRIP ${GNUCXX_VERSION} GNUCXX_VERSION)
	string(STRIP ${GNUCXX_TARGET_MACHINE} GNUCXX_TARGET_MACHINE)
	
	# Target machine
	if(GNUCXX_TARGET_MACHINE MATCHES "darwin")
		set(TOOLSET "xgcc")
	else()
		message(FATAL_ERROR "Detected unsupported GNUCXX target machine:\n  ${GNUCXX_TARGET_MACHINE}\nSupported GNUCXX target machines:\n  darwin\n")
	endif()

	# Version
	string(REPLACE "." "" GNUCXX_VERSION_FORMATTED ${GNUCXX_VERSION})
	set(TOOLSET ${TOOLSET}${GNUCXX_VERSION_FORMATTED})
	
	foreach(CONFIGURATION ${CONFIGURATIONS})
		if(${CMAKE_CXX_FLAGS_${CONFIGURATION}} MATCHES "-pthread")
			set(THREADING_${CONFIGURATION} mt)
		endif()
		if(${CMAKE_CXX_FLAGS_${CONFIGURATION}} MATCHES "-static-libgcc")
			set(ABI_${CONFIGURATION} ${ABI_${CONFIGURATION}}s-)
		endif()
		if(${CMAKE_CXX_FLAGS_${CONFIGURATION}} MATCHES "-g")
			set(ABI_${CONFIGURATION} ${ABI_${CONFIGURATION}}g-)
		endif()
	endforeach()


# ERROR
else()
	message(FATAL_ERROR "Unsupported toolset! Supported toolsets: MSVC (all versions), GNUCXX (darwin: all versions)")
endif()



##############################################################################
## Tag According to Configuration
##############################################################################
set(ABI_RELEASE ${ABI_RELEASE}release)
set(ABI_DEBUG ${ABI_DEBUG}debug)
set(ABI_MINSIZEREL ${ABI_MINSIZEREL}release_min_size)
set(ABI_RELWITHDEBINFO ${ABI_RELWITHDEBINFO}release_with_debug_info)
set(ABI_RELEASEWITHDEVELOPERTOOLS ${ABI_RELEASEWITHDEVELOPERTOOLS}release_with_developer_tools)
set(ABI_MINSIZERELWITHDEVELOPERTOOLS ${ABI_MINSIZERELWITHDEVELOPERTOOLS}release_min_size_with_developer_tools)



################################################################################
## Directory Structured Source Groups
##
## Organises files with EXTENSION into source groups named according to the
## directory structure of the file system. The organisation is recursive (it 
## visits subdirectories) starting in DIRECTORY.
##
## [Optional] The third parameter is used to specify an initial source group
## name. It defaults to DIRECTORY.
################################################################################
function(directory_structured_source_groups DIRECTORY EXTENSION)
	if(NOT ARGV2)
		set(ARGV2 ${DIRECTORY})
	endif()

	file(GLOB FILES "${DIRECTORY}/*.${EXTENSION}")
	source_group(${ARGV2} FILES ${FILES})

	file(GLOB SUBDIRECTORIES ${DIRECTORY}/*)
	foreach(SUBDIRECTORY ${SUBDIRECTORIES})
		if(IS_DIRECTORY ${SUBDIRECTORY})
			# Push to "stack" (enables recursion)
			string(REGEX MATCH "[a-zA-Z0-9_]*$" DIRECTORY_NAME ${SUBDIRECTORY})
			set(ARGV2 "${ARGV2}\\${DIRECTORY_NAME}")

			directory_structured_source_groups(${SUBDIRECTORY} ${EXTENSION} ${ARGV2})

			# Pop from "stack"
			string(REGEX REPLACE "\\\\[a-zA-Z0-9]*$" "" ARGV2 ${ARGV2})
		endif()

	endforeach()
endfunction()



function(set_target_output_properties TARGET_NAME TARGET_TYPE)
	set(RUNTIME_STAGE_DIR ${ARGV2})
	set(ARCHIVE_STAGE_DIR ${ARGV3})

	foreach(CONFIGURATION ${CONFIGURATIONS})

		# Output name
		set(OUTPUT_NAME_${CONFIGURATION} ${OUTPUT_NAME_${CONFIGURATION}}${TARGET_NAME}-${TOOLSET})
		if (NOT ${THREADING_${CONFIGURATION}} STREQUAL "")
			set(OUTPUT_NAME_${CONFIGURATION} ${OUTPUT_NAME_${CONFIGURATION}}-${THREADING_${CONFIGURATION}})
		endif()
		if (NOT ${ABI_${CONFIGURATION}} STREQUAL "")
			set(OUTPUT_NAME_${CONFIGURATION} ${OUTPUT_NAME_${CONFIGURATION}}-${ABI_${CONFIGURATION}})
		endif()
		if (NOT ${API_VERSION_NAME} STREQUAL "")
			set(OUTPUT_NAME_${CONFIGURATION} ${OUTPUT_NAME_${CONFIGURATION}}-${API_VERSION_NAME})
		endif()
		
		# Library (.dll, .so)
		if(${TARGET_TYPE} STREQUAL "LIBRARY")
			set_target_properties(${TARGET_NAME} PROPERTIES
				RUNTIME_OUTPUT_DIRECTORY_${CONFIGURATION} ${RUNTIME_STAGE_DIR}
				RUNTIME_OUTPUT_NAME_${CONFIGURATION} ${OUTPUT_NAME_${CONFIGURATION}}
				ARCHIVE_OUTPUT_DIRECTORY_${CONFIGURATION} ${ARCHIVE_STAGE_DIR}
				ARCHIVE_OUTPUT_NAME_${CONFIGURATION} ${PROJECT_NAME_LOWERCASE_UNDERSCORES}_${OUTPUT_NAME_${CONFIGURATION}}
				FOLDER ${PROJECT_NAME})

		# Archive (.lib, .a)
		elseif(${TARGET_TYPE} STREQUAL "ARCHIVE")
			set_target_properties(${TARGET_NAME} PROPERTIES
				ARCHIVE_OUTPUT_DIRECTORY_${CONFIGURATION} ${ARCHIVE_STAGE_DIR}
				ARCHIVE_OUTPUT_NAME_${CONFIGURATION} ${PROJECT_NAME_LOWERCASE_UNDERSCORES}_${OUTPUT_NAME_${CONFIGURATION}}
				FOLDER ${PROJECT_NAME})

		# Runtime (.exe)
		elseif(${TARGET_TYPE} STREQUAL "RUNTIME")
			set_target_properties(${TARGET_NAME} PROPERTIES
				RUNTIME_OUTPUT_DIRECTORY_${CONFIGURATION} ${RUNTIME_STAGE_DIR}
				RUNTIME_OUTPUT_NAME_${CONFIGURATION} ${OUTPUT_NAME_${CONFIGURATION}}
				FOLDER ${PROJECT_NAME})

		# Header-only (N/A)
		elseif(${TARGET_TYPE} STREQUAL "HEADER_ONLY")
			set_target_properties(${TARGET_NAME} PROPERTIES
				FOLDER ${PROJECT_NAME})

		# Test (.exe)
		elseif(${TARGET_TYPE} STREQUAL "TEST")
			set_target_properties(${TARGET_NAME} PROPERTIES
				RUNTIME_OUTPUT_DIRECTORY_${CONFIGURATION} ${RUNTIME_STAGE_DIR}
				RUNTIME_OUTPUT_NAME_${CONFIGURATION} ${OUTPUT_NAME_${CONFIGURATION}}
				FOLDER ${PROJECT_NAME}/Tests)
			
			add_test(
				NAME ${OUTPUT_NAME_${CONFIGURATION}}
				CONFIGURATIONS ${CONFIGURATION}
				WORKING_DIRECTORY ${RUNTIME_STAGE_DIR}
				COMMAND ${OUTPUT_NAME_${CONFIGURATION}})

		# ERROR
		else()
			message(FATAL_ERROR "Unsupported target type: ${TARGET_TYPE}. Supported entries: RUNTIME, LIBRARY, ARCHIVE, HEADER_ONLY, and TEST.")
		endif()
	endforeach()
endfunction()
