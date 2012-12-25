include(FindPackageHandleStandardArgs)



set(TINYXML_URL "https://sourceforge.net/projects/tinyxml/")



find_path(TINYXML_INCLUDE_DIR "tinyxml.h")
find_library(TINYXML_LIBRARY_RELEASE "tinyxml")
find_library(TINYXML_LIBRARY_DEBUG "tinyxmlD")



find_package_handle_standard_args(
	TINYXML 
	REQUIRED_VARS TINYXML_INCLUDE_DIR TINYXML_LIBRARY_RELEASE
	FAIL_MESSAGE "TINYXML was NOT found! TINYXML is available at:\n${TINYXML_URL}\nPlease attempt to resolve the following: ")
	
	
	
if(TINYXML_FOUND)
	if(TINYXML_LIBRARY_DEBUG)
		list(APPEND TINYXML_LIBRARIES debug ${TINYXML_LIBRARY_DEBUG})
		list(APPEND TINYXML_LIBRARIES optimized ${TINYXML_LIBRARY_RELEASE})
	endif()
	list(APPEND TINYXML_LIBRARIES general ${TINYXML_LIBRARY_RELEASE})
	set(TINYXML_LIBRARIES ${TINYXML_LIBRARIES} CACHE STRING "The accumulated libraries.")
	mark_as_advanced(TINYXML_LIBRARIES)
endif()