##############################################################################
## Configurations
##############################################################################
foreach(CONFIGURATION ${CMAKE_CONFIGURATION_TYPES})
	if(
		${CONFIGURATION} STREQUAL Debug OR
		${CONFIGURATION} STREQUAL Release OR
		${CONFIGURATION} STREQUAL MinSizeRel OR
		${CONFIGURATION} STREQUAL RelWithDebInfo
	)
		string(TOUPPER ${CONFIGURATION} CONFIGURATION_CAPITALIZED)	
		list(APPEND CONFIGURATIONS ${CONFIGURATION_CAPITALIZED})
	else()
		message(FATAL_ERROR "Unsupported configuration type: ${CONFIGURATION}")
	endif()
endforeach()



## Toolset
if (MSVC)
	math(EXPR VC_VERSION ${MSVC_VERSION}/10-60)
	set(TOOLSET vc${VC_VERSION})
	
	## Reference: http://msdn.microsoft.com/en-us/library/2kzt1wy3(v=vs.71).aspx
	foreach(CONFIGURATION ${CONFIGURATIONS})
		if(${CMAKE_CXX_FLAGS_${CONFIGURATION}} MATCHES "/MDd")
			set(THREADING_${CONFIGURATION} mt)
			set(ABI_${CONFIGURATION} ${ABI_${CONFIGURATION}}g)
		elseif(${CMAKE_CXX_FLAGS_${CONFIGURATION}} MATCHES "/MD")
			set(THREADING_${CONFIGURATION} mt)
		elseif(${CMAKE_CXX_FLAGS_${CONFIGURATION}} MATCHES "/MLd")
			set(ABI_${CONFIGURATION} ${ABI_${CONFIGURATION}}sg)
		elseif(${CMAKE_CXX_FLAGS_${CONFIGURATION}} MATCHES "/ML")
			set(ABI_${CONFIGURATION} ${ABI_${CONFIGURATION}}s)
		elseif(${CMAKE_CXX_FLAGS_${CONFIGURATION}} MATCHES "/MTd")
			set(THREADING_${CONFIGURATION} mt)
			set(ABI_${CONFIGURATION} ${ABI_${CONFIGURATION}}sg)
		elseif(${CMAKE_CXX_FLAGS_${CONFIGURATION}} MATCHES "/MT")
			set(THREADING_${CONFIGURATION} mt)
			set(ABI_${CONFIGURATION} ${ABI_${CONFIGURATION}}s)
		endif()
	endforeach()
elseif (CMAKE_COMPILER_IS_GNUCXX)
	## Identify compiler specifics
	execute_process(COMMAND ${CMAKE_CXX_COMPILER} "-dumpversion" OUTPUT_VARIABLE GNUCXX_VERSION)
	execute_process(COMMAND ${CMAKE_CXX_COMPILER} "-dumpmachine" OUTPUT_VARIABLE GNUCXX_TARGET_MACHINE)
	string(STRIP ${GNUCXX_VERSION} GNUCXX_VERSION)
	string(STRIP ${GNUCXX_TARGET_MACHINE} GNUCXX_TARGET_MACHINE)
	
	## Target machine
	if(GNUCXX_TARGET_MACHINE MATCHES "darwin")
		set(TOOLSET "xgcc")
	else()
		message(FATAL_ERROR "Detected unsupported GNUCXX target machine:\n  ${GNUCXX_TARGET_MACHINE}\nSupported GNUCXX target machines:\n  darwin\n")
	endif()

	## Version
	string(REPLACE "." "" GNUCXX_VERSION_FORMATTED ${GNUCXX_VERSION})
	set(TOOLSET ${TOOLSET}${GNUCXX_VERSION_FORMATTED})
	
	foreach(CONFIGURATION ${CONFIGURATIONS})
		if(${CMAKE_CXX_FLAGS_${CONFIGURATION}} MATCHES "-pthread")
			set(THREADING_${CONFIGURATION} mt)
		endif()
		if(${CMAKE_CXX_FLAGS_${CONFIGURATION}} MATCHES "-static-libgcc")
			set(ABI_${CONFIGURATION} ${ABI_${CONFIGURATION}}s)
		endif()
		if(${CMAKE_CXX_FLAGS_${CONFIGURATION}} MATCHES "-g")
			set(ABI_${CONFIGURATION} ${ABI_${CONFIGURATION}}g)
		endif()
	endforeach()
else()
	message(FATAL_ERROR "Unsupported toolset! Supported toolsets: MSVC (all versions), GNUCXX (darwin: all versions)")
endif()

## ABI based on configuration
set(ABI_RELEASE ${ABI_RELEASE})
set(ABI_DEBUG ${ABI_DEBUG}d)
set(ABI_MINSIZEREL ${ABI_MINSIZEREL}m)
set(ABI_RELWITHDEBINFO ${ABI_RELWITHDEBINFO}i)

## Definitions
if(WIN32)
	add_definitions("-DWINDOWS")
elseif(UNIX)
	add_definitions("-DUNIX")
endif()
if(APPLE)
	add_definitions("-DAPPLE")
endif()



################################################################################
# Functions
################################################################################
function(directory_source_group GROUP DIRECTORY EXTENSION)
	file(GLOB FILES "${DIRECTORY}/*.${EXTENSION}")
	source_group("${GROUP}" FILES ${FILES})

	file(GLOB SUBDIRECTORIES ${DIRECTORY}/*)
	foreach(SUBDIRECTORY ${SUBDIRECTORIES})
		if(IS_DIRECTORY ${SUBDIRECTORY})
			# Advance recursion
			string(REGEX MATCH "[a-zA-Z0-9_]*$" DIRECTORY_NAME ${SUBDIRECTORY})
			set(GROUP "${GROUP}\\${DIRECTORY_NAME}")

			directory_source_group(${GROUP} ${SUBDIRECTORY} ${EXTENSION})

			# Regress recursion
			string(REGEX REPLACE "\\\\[a-zA-Z0-9]*$" "" GROUP ${GROUP})
		endif()
	endforeach()
endfunction()

function(set_target_properties_output TARGET_NAME TARGET_TYPE RUNTIME_STAGE_DIR)
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
				RUNTIME_OUTPUT_NAME_${CONFIGURATION} ${OUTPUT_NAME_${CONFIGURATION}}
				ARCHIVE_OUTPUT_DIRECTORY_${CONFIGURATION} ${PROJECT_SOURCE_DIR}/stage/libraries
				ARCHIVE_OUTPUT_NAME_${CONFIGURATION} ${PROJECT_NAME_LOWERCASE_UNDERSCORES}_${OUTPUT_NAME_${CONFIGURATION}}
				FOLDER ${PROJECT_NAME}
			)
			
			if(NOT ${RUNTIME_STAGE_DIR} STREQUAL "DEFAULT")
				set_target_properties(${TARGET_NAME} PROPERTIES
					RUNTIME_OUTPUT_DIRECTORY_${CONFIGURATION} ${RUNTIME_STAGE_DIR}
				)
			else()
				set_target_properties(${TARGET_NAME} PROPERTIES
					RUNTIME_OUTPUT_DIRECTORY_${CONFIGURATION} ${PROJECT_SOURCE_DIR}/stage/binaries
				)
			endif()
		# Archive (.lib)
		elseif(${TARGET_TYPE} STREQUAL "ARCHIVE")
			set_target_properties(${TARGET_NAME} PROPERTIES
				ARCHIVE_OUTPUT_DIRECTORY_${CONFIGURATION} ${PROJECT_SOURCE_DIR}/stage/libraries
				ARCHIVE_OUTPUT_NAME_${CONFIGURATION} ${PROJECT_NAME_LOWERCASE_UNDERSCORES}_${OUTPUT_NAME_${CONFIGURATION}}
				FOLDER ${PROJECT_NAME}
			)
		# Runtime (.exe)
		elseif(${TARGET_TYPE} STREQUAL "RUNTIME")
			set_target_properties(${TARGET_NAME} PROPERTIES
				RUNTIME_OUTPUT_DIRECTORY_${CONFIGURATION} ${PROJECT_SOURCE_DIR}/stage/binaries
				RUNTIME_OUTPUT_NAME_${CONFIGURATION} ${OUTPUT_NAME_${CONFIGURATION}}
				FOLDER ${PROJECT_NAME}
			)
		# Header-only (N/A)
		elseif(${TARGET_TYPE} STREQUAL "HEADER_ONLY")
			set_target_properties(${TARGET_NAME} PROPERTIES
				FOLDER ${PROJECT_NAME}
			)
		# Test (.exe)
		elseif(${TARGET_TYPE} STREQUAL "TEST")
			set_target_properties(${TARGET_NAME} PROPERTIES
				RUNTIME_OUTPUT_NAME_${CONFIGURATION} ${OUTPUT_NAME_${CONFIGURATION}}
				FOLDER ${PROJECT_NAME}/Tests
			)
			
			if(NOT ${RUNTIME_STAGE_DIR} STREQUAL "DEFAULT")
				set_target_properties(${TARGET_NAME} PROPERTIES
					RUNTIME_OUTPUT_DIRECTORY_${CONFIGURATION} ${RUNTIME_STAGE_DIR}
				)
			else()
				set_target_properties(${TARGET_NAME} PROPERTIES
					RUNTIME_OUTPUT_DIRECTORY_${CONFIGURATION} ${PROJECT_SOURCE_DIR}/stage/binaries
				)
			endif()
			
			add_test(
				NAME ${OUTPUT_NAME_${CONFIGURATION}}
				CONFIGURATIONS ${CONFIGURATION}
				WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/stage/binaries
				COMMAND ${OUTPUT_NAME_${CONFIGURATION}}
				)
		# ERROR
		else()
			message(FATAL_ERROR "Unsupported target type: ${TARGET_TYPE}. Supported entries: RUNTIME, LIBRARY, ARCHIVE, HEADER_ONLY, and TEST.")
		endif()
	endforeach()
endfunction()