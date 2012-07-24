set(BlackLabel_FOUND TRUE)
set(BlackLabel_IN_CACHE TRUE)

##############################################################################
## Functions
##############################################################################
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



##############################################################################
## Directories
##############################################################################

## If library and include directories must be deduced
if(NOT BlackLabel_LIBRARY_DIR OR NOT BlackLabel_INCLUDE_DIR)
	set(BlackLabel_IN_CACHE FALSE)

	## If root directory must be deduced
	if(NOT BlackLabel_ROOT)
		## Try to deduce root diretory
		if(IS_DIRECTORY ${CMAKE_SOURCE_DIR}/black_label)
			set(BlackLabel_ROOT "${CMAKE_SOURCE_DIR}/black_label")
		endif()
	endif()
	if(BlackLabel_ROOT)
		set(BlackLabel_LIBRARY_DIR ${BlackLabel_ROOT}/stage/libraries CACHE PATH "Black Label library directory.")
		set(BlackLabel_INCLUDE_DIR ${BlackLabel_ROOT} CACHE PATH "Black Label include directory.")
		mark_as_advanced(FORCE BlackLabel_LIBRARY_DIR BlackLabel_INCLUDE_DIR)
	else()
		set(BlackLabel_FOUND FALSE)
		set(BlackLabel_ERROR_RESOLUTION "Please specify either:\n  The root directory: BlackLabel_ROOT\n  OR\n  Both library and include directories: BlackLabel_LIBRARY_DIR and BlackLabel_INCLUDE_DIR")
	endif()
endif()

set(BlackLabel_INCLUDE_DIRS ${BlackLabel_INCLUDE_DIR})



##############################################################################
## Components
##############################################################################
if(BlackLabel_FOUND)
	foreach(COMPONENT ${BlackLabel_FIND_COMPONENTS})
		string(REGEX MATCH "^[^ ]*" COMPONENT_NAME ${COMPONENT})
		string(REGEX MATCH " .*$" COMPONENT_VERSION ${COMPONENT})
		string(STRIP ${COMPONENT_NAME} COMPONENT_NAME)
		
		if (NOT COMPONENT_VERSION)
			set(BlackLabel_FOUND FALSE)
			set(BlackLabel_ERROR_RESOLUTION "Black Label component ${COMPONENT_NAME} is missing its version number! The syntax is:\n  <component> <version> [<component> <version> ...]")
			break()
		endif()
		
		string(TOUPPER ${COMPONENT_NAME} COMPONENT_NAME_CAPITALIZED)
		string(STRIP ${COMPONENT_VERSION} COMPONENT_VERSION)

		if(WIN32)
			set(PREFIX "")
			set(EXTENSION ".lib")
		elseif(UNIX)
			set(PREFIX "lib")
			set(EXTENSION ".a")
		else()
			set(BlackLabel_FOUND FALSE)
			set(BlackLabel_ERROR_RESOLUTION "Unknown target platform! Known platforms:\n  Windows, Unix, and Unix like")
			break()
		endif()
		
		black_label_version_suffix(RELEASE ${COMPONENT_VERSION} SUFFIX)
		set(BlackLabel_${COMPONENT_NAME_CAPITALIZED}_RELEASE ${BlackLabel_LIBRARY_DIR}/${PREFIX}black_label_${COMPONENT_NAME}${SUFFIX}${EXTENSION} CACHE PATH "Release version of the ${COMPONENT_NAME} library.")
		black_label_version_suffix(DEBUG ${COMPONENT_VERSION} SUFFIX)
		set(BlackLabel_${COMPONENT_NAME_CAPITALIZED}_DEBUG ${BlackLabel_LIBRARY_DIR}/${PREFIX}black_label_${COMPONENT_NAME}${SUFFIX}${EXTENSION} CACHE PATH "Debug version of the ${COMPONENT_NAME} library.")
		
		set(
			BlackLabel_${COMPONENT_NAME_CAPITALIZED}_LIBRARIES
			optimized	${BlackLabel_${COMPONENT_NAME_CAPITALIZED}_RELEASE}
			debug		${BlackLabel_${COMPONENT_NAME_CAPITALIZED}_DEBUG}
			)
		
		list(APPEND BlackLabel_LIBRARIES ${BlackLabel_${COMPONENT_NAME_CAPITALIZED}_LIBRARIES})
			
		mark_as_advanced(
			FORCE
			BlackLabel_${COMPONENT_NAME_CAPITALIZED}_RELEASE
			BlackLabel_${COMPONENT_NAME_CAPITALIZED}_DEBUG
			)
	endforeach()
endif()



##############################################################################
## Report
##############################################################################
if(NOT BlackLabel_FIND_QUIETLY)
	if(NOT BlackLabel_FOUND)
		if(BlackLabel_FIND_REQUIRED)
			message(SEND_ERROR "Could not find the BlackLabel package! ${BlackLabel_ERROR_RESOLUTION}")
		else()
			message(STATUS "Could not find the BlackLabel package! ${BlackLabel_ERROR_RESOLUTION}")
		endif()
	elseif(NOT BlackLabel_IN_CACHE)
		message(STATUS "Found BlackLabel with the following components:")
		foreach(COMPONENT ${BlackLabel_FIND_COMPONENTS})
			message(STATUS "  ${COMPONENT}")
		endforeach()
	endif()
endif()