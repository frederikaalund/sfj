include(FindPackageHandleStandardArgs)



set(FBX_URL "http://usa.autodesk.com/adsk/servlet/pc/item?siteID=123112&id=10775847")



if(WIN32)
	find_path(FBX_INCLUDE_DIR "fbxsdk.h")
	find_library(FBX_LIBRARY_RELEASE "FBX")
	find_library(FBX_LIBRARY_DEBUG "FBXD")
elseif(APPLE)
	set(FBX_INCLUDE_DIR_POTENTIAL_1 "/Applications/Autodesk/FBX SDK/2013.3/include")
	set(FBX_LIBRARY_DIR_POTENTIAL_1 "/Applications/Autodesk/FBX SDK/2013.3/lib/gcc4/ub")

	find_path(FBX_INCLUDE_DIR "fbxsdk.h"
		PATHS ${FBX_INCLUDE_DIR_POTENTIAL_1})
	find_library(FBX_LIBRARY_RELEASE "libfbxsdk-2013.3-static.a"
		PATH ${FBX_LIBRARY_DIR_POTENTIAL_1})
	find_library(FBX_LIBRARY_DEBUG "libfbxsdk-2013.3-staticd.a"
		PATH ${FBX_LIBRARY_DIR_POTENTIAL_1})
endif()



find_package_handle_standard_args(
	FBX 
	REQUIRED_VARS FBX_INCLUDE_DIR FBX_LIBRARY_RELEASE FBX_LIBRARY_DEBUG
	FAIL_MESSAGE "FBX was NOT found! FBX is available at:\n${FBX_URL}\nPlease attempt to resolve the following: ")
	
	
	
if(FBX_FOUND)
	set(FBX_LIBRARIES debug ${FBX_LIBRARY_DEBUG} optimized ${FBX_LIBRARY_RELEASE} CACHE STRING "The accumulated libraries.")
	mark_as_advanced(FBX_LIBRARIES)
endif()