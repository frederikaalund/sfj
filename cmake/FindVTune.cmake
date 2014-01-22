include(FindPackageHandleStandardArgs)



set(VTune_URL "http://software.intel.com/en-us/intel-vtune-amplifier-xe")



find_path(VTune_INCLUDE_DIR "ittnotify.h")
find_library(VTune_LIBRARY_RELEASE "libittnotify")



find_package_handle_standard_args(
	VTune 
	REQUIRED_VARS VTune_INCLUDE_DIR VTune_LIBRARY_RELEASE
	FAIL_MESSAGE "VTune was NOT found! VTune can be bought at:\n${VTune_URL}\nPlease attempt to resolve the following: ")
	
	
	
if(VTUNE_FOUND)
	set(VTune_LIBRARIES ${VTune_LIBRARY_RELEASE} CACHE STRING "The VTune library.")
	mark_as_advanced(VTune_LIBRARIES)
endif()