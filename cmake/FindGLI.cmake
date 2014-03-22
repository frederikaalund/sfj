include(FindPackageHandleStandardArgs)



set(GLI_URL "http://gli.g-truc.net/0.5.1/")



find_path(GLI_INCLUDE_DIR "gli/gli.hpp")



find_package_handle_standard_args(
	GLI 
	REQUIRED_VARS GLI_INCLUDE_DIR
	FAIL_MESSAGE "GLI was NOT found! GLI is available at:\n${GLM_URL}\nPlease attempt to resolve the following: ")