include(FindPackageHandleStandardArgs)



set(OPENCOLORIO_URL "http://opencolorio.org/downloads.html")



find_path(OPENCOLORIO_INCLUDE_DIR "OPENCOLORIO/ai_assert.h")
find_library(OPENCOLORIO_LIBRARY_RELEASE "OPENCOLORIO")
find_library(OPENCOLORIO_LIBRARY_DEBUG "OPENCOLORIOD")



find_package_handle_standard_args(
	OPENCOLORIO 
	REQUIRED_VARS OPENCOLORIO_INCLUDE_DIR OPENCOLORIO_LIBRARY_RELEASE OPENCOLORIO_LIBRARY_DEBUG
	FAIL_MESSAGE "OPENCOLORIO was NOT found! OPENCOLORIO is available at:\n${OPENCOLORIO_URL}\nPlease attempt to resolve the following: ")
	
	
	
if(OPENCOLORIO_FOUND)
	set(OPENCOLORIO_LIBRARIES debug ${OPENCOLORIO_LIBRARY_DEBUG} optimized ${OPENCOLORIO_LIBRARY_RELEASE} CACHE STRING "The accumulated libraries.")
	mark_as_advanced(OPENCOLORIO_LIBRARIES)
endif()