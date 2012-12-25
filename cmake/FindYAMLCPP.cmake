include(FindPackageHandleStandardArgs)



set(YAMLCPP_URL "http://code.google.com/p/yaml-cpp/downloads/list")



find_path(YAMLCPP_INCLUDE_DIR "yaml-cpp/yaml.h")
find_library(YAMLCPP_LIBRARY_RELEASE "yaml-cpp")
find_library(YAMLCPP_LIBRARY_DEBUG "yaml-cpp")



find_package_handle_standard_args(
	YAMLCPP 
	REQUIRED_VARS YAMLCPP_INCLUDE_DIR YAMLCPP_LIBRARY_RELEASE YAMLCPP_LIBRARY_DEBUG
	FAIL_MESSAGE "YAMLCPP was NOT found! YAMLCPP is available at:\n${YAMLCPP_URL}\nPlease attempt to resolve the following: ")
	
	
	
if(YAMLCPP_FOUND)
	set(YAMLCPP_LIBRARIES debug ${YAMLCPP_LIBRARY_DEBUG} optimized ${YAMLCPP_LIBRARY_RELEASE} CACHE STRING "The accumulated libraries.")
	mark_as_advanced(YAMLCPP_LIBRARIES)
endif()