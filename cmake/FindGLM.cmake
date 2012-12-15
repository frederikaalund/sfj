include(FindPackageHandleStandardArgs)



set(GLM_URL "http://glm.g-truc.net/download.html")



find_path(GLM_INCLUDE_DIR "glm/glm.hpp")



find_package_handle_standard_args(
	GLM 
	REQUIRED_VARS GLM_INCLUDE_DIR
	FAIL_MESSAGE "GLM was NOT found! GLM is available at:\n${GLM_URL}\nPlease attempt to resolve the following: ")