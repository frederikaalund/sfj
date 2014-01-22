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

if(WIN32)
	set(DEVELOPER_TOOLS_DEFINE "/DDEVELOPER_TOOLS")
else()
	set(DEVELOPER_TOOLS_DEFINE "-DDEVELOPER_TOOLS")
endif()

add_build_type_and_configuration(
	"DebugWithDeveloperTools"
	"Debug"
	${DEVELOPER_TOOLS_DEFINE})

add_build_type_and_configuration(
	"ReleaseWithDeveloperTools"
	"Release"
	${DEVELOPER_TOOLS_DEFINE})
add_build_type_and_configuration(
	"MinSizeRelWithDeveloperTools"
	"MinSizeRel"
	${DEVELOPER_TOOLS_DEFINE})



##############################################################################
## Identify Configurations
##############################################################################
foreach(CONFIGURATION ${CMAKE_CONFIGURATION_TYPES})
	if(
		${CONFIGURATION} STREQUAL Debug OR
		${CONFIGURATION} STREQUAL Release OR
		${CONFIGURATION} STREQUAL MinSizeRel OR
		${CONFIGURATION} STREQUAL RelWithDebInfo OR
		${CONFIGURATION} STREQUAL DebugWithDeveloperTools OR
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
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC" AND NOT MSVC_VERSION LESS 1600)
	# Query if there is a toolset override
	if(CMAKE_GENERATOR_TOOLSET)
		string(SUBSTRING ${CMAKE_GENERATOR_TOOLSET} 1 3 VC_VERSION)
	# Else the toolset must match the MSVC version
	else()
		math(EXPR VC_VERSION ${MSVC_VERSION}/10-60)
	endif()
	set(TOOLSET vc${VC_VERSION})

	# Setup configurations
	# Reference: http://msdn.microsoft.com/en-us/library/2kzt1wy3(v=vs.100).aspx
	foreach(CONFIGURATION ${CONFIGURATIONS})
		if(${CMAKE_CXX_FLAGS_${CONFIGURATION}} MATCHES "/MDd")
			set(ABI_${CONFIGURATION} ${ABI_${CONFIGURATION}}g-)
		# This test must be done before the "/MT" test
		elseif(${CMAKE_CXX_FLAGS_${CONFIGURATION}} MATCHES "/MTd")
			set(ABI_${CONFIGURATION} ${ABI_${CONFIGURATION}}sg-)
		elseif(${CMAKE_CXX_FLAGS_${CONFIGURATION}} MATCHES "/MT")
			set(ABI_${CONFIGURATION} ${ABI_${CONFIGURATION}}s-)
		endif()
	endforeach()



# Clang
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	# Query information
	execute_process(COMMAND ${CMAKE_CXX_COMPILER} "--version" OUTPUT_VARIABLE CLANG_INFO)
	string(STRIP ${CLANG_INFO} CLANG_INFO)
	
	# Toolset vendor
	if(CLANG_INFO MATCHES "Apple clang")
		set(TOOLSET "apple_clang")
	else()
		set(TOOLSET "clang")
	endif()	

	# Version
	string(REPLACE "." "" CLANG_VERSION_FORMATTED ${CMAKE_CXX_COMPILER_VERSION})
	set(TOOLSET ${TOOLSET}${CLANG_VERSION_FORMATTED})

	# Setup configurations
	# Reference: http://clang.llvm.org/docs/UsersManual.html
	foreach(CONFIGURATION ${CONFIGURATIONS})
		if(${CMAKE_CXX_FLAGS_${CONFIGURATION}} MATCHES "-g")
			set(ABI_${CONFIGURATION} ${ABI_${CONFIGURATION}}g-)
		endif()
	endforeach()



# ERROR
else()
	message(FATAL_ERROR "Unsupported toolset: ${CMAKE_CXX_COMPILER_ID}! Supported toolsets: MSVC (MSVC1600 and above / VS2010 and above), Clang")
endif()



##############################################################################
## Tag According to Configuration
##############################################################################
set(ABI_RELEASE ${ABI_RELEASE}release)
set(ABI_DEBUG ${ABI_DEBUG}debug)
set(ABI_MINSIZEREL ${ABI_MINSIZEREL}release_min_size)
set(ABI_RELWITHDEBINFO ${ABI_RELWITHDEBINFO}release_with_debug_info)
set(ABI_DEBUGWITHDEVELOPERTOOLS ${ABI_DEBUGWITHDEVELOPERTOOLS}debug_with_developer_tools)
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
			string(REGEX REPLACE "\\\\[a-zA-Z0-9_]*$" "" ARGV2 ${ARGV2})
		endif()

	endforeach()
endfunction()



function(set_target_output_properties TARGET_NAME TARGET_TYPE)
	set(RUNTIME_STAGE_DIR ${ARGV2})
	set(ARCHIVE_STAGE_DIR ${ARGV3})

	foreach(CONFIGURATION ${CONFIGURATIONS})

		# Output name
		set(OUTPUT_NAME_${CONFIGURATION} ${OUTPUT_NAME_${CONFIGURATION}}${TARGET_NAME}-${TOOLSET})
		if(NOT ${ABI_${CONFIGURATION}} STREQUAL "")
			set(OUTPUT_NAME_${CONFIGURATION} ${OUTPUT_NAME_${CONFIGURATION}}-${ABI_${CONFIGURATION}})
		endif()
		if(NOT ${API_VERSION_NAME} STREQUAL "")
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
