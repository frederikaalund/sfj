include(FindPackageHandleStandardArgs)



set(ASSIMP_URL "http://assimp.sourceforge.net/main_downloads.html")



find_path(ASSIMP_INCLUDE_DIR "assimp/ai_assert.h")
find_library(ASSIMP_LIBRARY_RELEASE "assimp")
find_library(ASSIMP_LIBRARY_DEBUG "assimpD")



find_package_handle_standard_args(
	ASSIMP 
	REQUIRED_VARS ASSIMP_INCLUDE_DIR ASSIMP_LIBRARY_RELEASE ASSIMP_LIBRARY_DEBUG
	FAIL_MESSAGE "Assimp was NOT found! Assimp is available at:\n${ASSIMP_URL}\nPlease attempt to resolve the following: ")
	
	
	
if(ASSIMP_FOUND)
	set(ASSIMP_LIBRARIES debug ${ASSIMP_LIBRARY_DEBUG} optimized ${ASSIMP_LIBRARY_RELEASE} CACHE STRING "The accumulated libraries.")
	mark_as_advanced(ASSIMP_LIBRARIES)
endif()