if(COMPONENT_CAPITALIZED)
	set(COMPONENT_CAPITALIZED_SAVED ${COMPONENT_CAPITALIZED})
endif()



include(FindPackageHandleStandardArgs)



function(black_label_version_suffix CONFIGURATION API_VERSION VERSION_SUFFIX)
	set(VERSION_SUFFIX_LOCAL -${TOOLSET})
	if (NOT ${ABI_${CONFIGURATION}} STREQUAL "")
		set(VERSION_SUFFIX_LOCAL ${VERSION_SUFFIX_LOCAL}-${ABI_${CONFIGURATION}})
	endif()
	if (NOT ${API_VERSION} STREQUAL "")
		string(REPLACE "." "_" API_VERSION_FORMATTED ${API_VERSION})
		set(VERSION_SUFFIX_LOCAL ${VERSION_SUFFIX_LOCAL}-${API_VERSION_FORMATTED})
	endif()
	set(${VERSION_SUFFIX} ${VERSION_SUFFIX_LOCAL} PARENT_SCOPE)
endfunction()



find_path(BlackLabel_INCLUDE_DIR "black_label/renderer.hpp"
	HINTS ${SFJ_CMAKE_DIR}/../black_label)



if(BlackLabel_INCLUDE_DIR)

	list(APPEND BlackLabel_INCLUDE_DIRS ${BlackLabel_INCLUDE_DIR})

	set(BlackLabel_LIBRARY_DIR "${BlackLabel_INCLUDE_DIR}/stage/libraries" CACHE PATH "Black Label library directory.")
	set(BlackLabel_CMAKE_DIR "${BlackLabel_INCLUDE_DIR}/cmake" CACHE PATH "Black Label cmake directory.")
	
	mark_as_advanced(BlackLabel_LIBRARY_DIR)



	foreach(COMPONENT ${BlackLabel_FIND_COMPONENTS})
		string(TOUPPER ${COMPONENT} COMPONENT_CAPITALIZED)
		
	
		
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



		if(NOT TARGET BlackLabel_${COMPONENT_CAPITALIZED})
			add_library(BlackLabel_${COMPONENT_CAPITALIZED} UNKNOWN IMPORTED)

			foreach(CONFIGURATION ${CONFIGURATIONS})
				black_label_version_suffix(${CONFIGURATION} ${BlackLabel_FIND_VERSION} SUFFIX)
				set(BlackLabel_${COMPONENT_CAPITALIZED}_${CONFIGURATION} ${BlackLabel_LIBRARY_DIR}/${PREFIX}black_label_${COMPONENT}${SUFFIX}${EXTENSION} CACHE PATH "${CONFIGURATION} version of the ${COMPONENT} library.")
				set_target_properties(
					BlackLabel_${COMPONENT_CAPITALIZED} PROPERTIES
					IMPORTED_LOCATION_${CONFIGURATION} ${BlackLabel_${COMPONENT_CAPITALIZED}_${CONFIGURATION}})
				mark_as_advanced(BlackLabel_${COMPONENT_CAPITALIZED}_${CONFIGURATION})
			endforeach()
		endif()
		list(APPEND BlackLabel_LIBRARIES BlackLabel_${COMPONENT_CAPITALIZED})
			


		set(BlackLabel_${COMPONENT}_FOUND TRUE)
			
		
			
		if(BlackLabel_USE_STATIC_LIBS)
			set(${COMPONENT}_DEPENDENCIES_FILE ${BlackLabel_CMAKE_DIR}/dependencies_${COMPONENT}.cmake)
			if(EXISTS ${${COMPONENT}_DEPENDENCIES_FILE})
				include(${${COMPONENT}_DEPENDENCIES_FILE})		
				list(APPEND BlackLabel_INCLUDE_DIRS ${BlackLabel_DEPENDENCIES_${COMPONENT_CAPITALIZED}_INCLUDE_DIRS})
				list(APPEND BlackLabel_LIBRARIES ${BlackLabel_DEPENDENCIES_${COMPONENT_CAPITALIZED}_LIBRARIES})		
			endif()
		endif()
	endforeach()
	

			
	if(BlackLabel_USE_STATIC_LIBS)
		include(${BlackLabel_CMAKE_DIR}/dependencies_common.cmake)
		list(APPEND BlackLabel_INCLUDE_DIRS ${BlackLabel_DEPENDENCIES_COMMON_INCLUDE_DIRS})
		list(APPEND BlackLabel_LIBRARIES ${BlackLabel_DEPENDENCIES_COMMON_LIBRARIES})
	endif()
endif()



find_package_handle_standard_args(
	BlackLabel
	REQUIRED_VARS BlackLabel_INCLUDE_DIR
	HANDLE_COMPONENTS)
	
	
	
if(COMPONENT_CAPITALIZED_SAVED)
	set(COMPONENT_CAPITALIZED ${COMPONENT_CAPITALIZED_SAVED})
	unset(COMPONENT_CAPITALIZED_SAVED)
endif()