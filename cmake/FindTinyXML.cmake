include(FindPackageHandleStandardArgs)



set(TINYXML_URL "http://TINYXML.org/downloads.html")



find_path(TINYXML_INCLUDE_DIR "TINYXML/ai_assert.h")
find_library(TINYXML_LIBRARY_RELEASE "TINYXML")
find_library(TINYXML_LIBRARY_DEBUG "TINYXMLD")



find_package_handle_standard_args(
	TINYXML 
	REQUIRED_VARS TINYXML_INCLUDE_DIR TINYXML_LIBRARY_RELEASE TINYXML_LIBRARY_DEBUG
	FAIL_MESSAGE "TINYXML was NOT found! TINYXML is available at:\n${TINYXML_URL}\nPlease attempt to resolve the following: ")
	
	
	
if(TINYXML_FOUND)
	set(TINYXML_LIBRARIES debug ${TINYXML_LIBRARY_DEBUG} optimized ${TINYXML_LIBRARY_RELEASE} CACHE STRING "The accumulated libraries.")
	mark_as_advanced(TINYXML_LIBRARIES)
endif()