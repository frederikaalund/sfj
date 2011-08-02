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



if (MSVC)
	## Toolset
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
	
	## ABI based on configuration
	set(ABI_RELEASE ${ABI_RELEASE})
	set(ABI_DEBUG ${ABI_DEBUG}d)
	set(ABI_MINSIZEREL ${ABI_MINSIZEREL}m)
	set(ABI_RELWITHDEBINFO ${ABI_RELWITHDEBINFO}i)
else()
	message(FATAL_ERROR "Unsupported toolset! Supported toolsets: MSVC (all versions)")
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
			string(REGEX MATCH "[a-zA-Z0-9]*$" DIRECTORY_NAME ${SUBDIRECTORY})
			set(GROUP "${GROUP}\\${DIRECTORY_NAME}")
		
			directory_source_group(${GROUP} ${SUBDIRECTORY} ${EXTENSION})

			# Regress recursion
			string(REGEX REPLACE "\\\\[a-zA-Z0-9]*$" "" GROUP ${GROUP})
		endif()
	endforeach()
endfunction()

function(set_target_properties_output TARGET_NAME TARGET_TYPE)
	if(${TARGET_TYPE} STREQUAL "RUNTIME")
		set(RUNTIME TRUE)
	elseif(
		${TARGET_TYPE} STREQUAL "LIBRARY" OR
		${TARGET_TYPE} STREQUAL "ARCHIVE"
		)
	else()
		message(FATAL_ERROR "Unsupported target type: ${TARGET_TYPE}. Supported entries: RUNTIME, LIBRARY, and ARCHIVE.")
	endif()

	foreach(CONFIGURATION ${CONFIGURATIONS})
		if(NOT RUNTIME)
			set(OUTPUT_NAME_${CONFIGURATION} ${PROJECT_NAME}_)
		endif()
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
		
		if(RUNTIME)
			set(STAGE_DIR binaries)
		else()
			set(STAGE_DIR libraries)
		endif()

		set_target_properties(${TARGET_NAME} PROPERTIES
			${TARGET_TYPE}_OUTPUT_DIRECTORY_${CONFIGURATION} ${PROJECT_SOURCE_DIR}/stage/${STAGE_DIR}
			${TARGET_TYPE}_OUTPUT_NAME_${CONFIGURATION} ${OUTPUT_NAME_${CONFIGURATION}}
		)
	endforeach()
endfunction()