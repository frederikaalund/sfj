#ifndef BLACK_LABEL_RENDERER_CAMERA_HPP
#define BLACK_LABEL_RENDERER_CAMERA_HPP

#include <black_label/shared_library/utility.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>



namespace black_label
{
namespace renderer
{

class BLACK_LABEL_SHARED_LIBRARY camera
{
public:
	camera() {}

	camera( const glm::vec3 eye, const glm::vec3 target, const glm::vec3 sky )
		: eye(eye)
		, target(target)
		, sky(sky), fovy(45.0f)
		, z_near(1.0f)
		, z_far(10000.0f)
	{ on_camera_moved(); }

	void strafe( glm::vec3 delta_in_camera_space );
	void pan( float azimuth_delta, float inclination_delta );

	void on_camera_moved();
	void on_window_resized( int width, int height );

	glm::vec3 forward() { return glm::vec3(-view_matrix[0][2], -view_matrix[1][2], -view_matrix[2][2]); }
	glm::vec3 right() { return glm::vec3(view_matrix[0][0], view_matrix[1][0], view_matrix[2][0]); }
	glm::vec3 up() { return glm::vec3(view_matrix[0][1], view_matrix[1][1], view_matrix[2][1]); }

  

MSVC_PUSH_WARNINGS(4251)
  
	glm::mat4 view_matrix, projection_matrix, view_projection_matrix;
	glm::vec3 eye, target, sky;

	glm::ivec2 window;
	glm::vec2 window_f;
	
	float 
		fovy,
		aspect_ratio,
		z_near,
		z_far;

MSVC_POP_WARNINGS()
};

} // namespace renderer
} // namespace black_label



#endif