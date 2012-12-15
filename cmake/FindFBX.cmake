include(FindPackageHandleStandardArgs)



set(FBX_URL "http://usa.autodesk.com/adsk/servlet/pc/item?siteID=123112&id=10775847")



find_path(FBX_INCLUDE_DIR "FBX/ai_assert.h")
find_library(FBX_LIBRARY_RELEASE "FBX")
find_library(FBX_LIBRARY_DEBUG "FBXD")



find_package_handle_standard_args(
	FBX 
	REQUIRED_VARS FBX_INCLUDE_DIR FBX_LIBRARY_RELEASE FBX_LIBRARY_DEBUG
	FAIL_MESSAGE "FBX was NOT found! FBX is available at:\n${FBX_URL}\nPlease attempt to resolve the following: ")
	
	
	
if(FBX_FOUND)
	set(FBX_LIBRARIES debug ${FBX_LIBRARY_DEBUG} optimized ${FBX_LIBRARY_RELEASE} CACHE STRING "The accumulated libraries.")
	mark_as_advanced(FBX_LIBRARIES)
endif()