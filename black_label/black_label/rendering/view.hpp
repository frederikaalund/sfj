#ifndef BLACK_LABEL_RENDERING_VIEW_HPP
#define BLACK_LABEL_RENDERING_VIEW_HPP

#include <black_label/shared_library/utility.hpp>
#include <black_label/utility/warning_suppression.hpp>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>



namespace black_label {
namespace rendering {

class BLACK_LABEL_SHARED_LIBRARY view
{
public:
	enum projection { perspective, orthographic, none };

	view() {}

	// None
	view( int width, int height ) 
		: projection{none}
	{ on_window_resized(width, height); }

	// Orthographic
	view( 
		const glm::vec3 eye,
		const glm::vec3 target, 
		const glm::vec3 sky,
		float left,
		float right_,
		float bottom,
		float top,
		float z_near = 10.0f,
		float z_far = 10000.0f )
		: projection{orthographic}
		, eye{eye}
		, target{target}
		, sky{sky}
		, left{left}
		, right_{right_}
		, bottom{bottom}
		, top{top}
		, z_near{z_near}
		, z_far{z_far}
	{ on_view_moved(); }

	// Orthographic
	view( 
		const glm::vec3 eye,
		const glm::vec3 target, 
		const glm::vec3 sky,
		int width,
		int height,
		float left,
		float right_,
		float bottom,
		float top,
		float z_near = 10.0f,
		float z_far = 10000.0f )
		: view{eye, target, sky, left, right_, bottom, top, z_near, z_far}
	{ on_window_resized(width, height); }

	// Perspective
	view( 
		const glm::vec3 eye,
		const glm::vec3 target, 
		const glm::vec3 sky,
		float z_near = 10.0f,
		float z_far = 10000.0f )
		: projection{perspective}
		, eye{eye}
		, target{target}
		, sky{sky}
		, fovy{glm::radians(45.0f)}
		, z_near{z_near}
		, z_far{z_far}
	{ on_view_moved(); }

	view( 
		const glm::vec3 eye,
		const glm::vec3 target, 
		const glm::vec3 sky,
		int width,
		int height,
		float z_near = 10.0f,
		float z_far = 10000.0f )
		: view{eye, target, sky, z_near, z_far}
	{ on_window_resized(width, height); }

	void strafe( glm::vec3 delta_in_view_space );
	void pan( float azimuth_delta, float inclination_delta );

	void on_view_moved();
	void on_window_resized( int width, int height );

	glm::vec3 forward() const { return glm::vec3{-view_matrix[0][2], -view_matrix[1][2], -view_matrix[2][2]}; }
	glm::vec3 right() const { return glm::vec3{view_matrix[0][0], view_matrix[1][0], view_matrix[2][0]}; }
	glm::vec3 up() const { return glm::vec3{view_matrix[0][1], view_matrix[1][1], view_matrix[2][1]}; }

  

MSVC_PUSH_WARNINGS(4251)
  
	projection projection;

	glm::mat4 view_matrix, projection_matrix, view_projection_matrix;
	glm::vec3 eye, target, sky;

	glm::ivec2 window;
	glm::vec2 window_f;
	
	// TODO: Divide into a union (orthographic / perspective)
	float
		left,
		right_,
		bottom,
		top,
		fovy,
		aspect_ratio,
		z_near,
		z_far;

MSVC_POP_WARNINGS()
};

} // namespace rendering
} // namespace black_label



#endif