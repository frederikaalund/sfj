#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/rendering/view.hpp>



namespace black_label {
namespace rendering {

void view::strafe( glm::vec3 delta_in_view_space )
{
	glm::mat3 matrix_sub3x3{view_matrix};
	glm::vec3 delta_in_world_space(
		glm::vec3(delta_in_view_space[0], 0.0f, delta_in_view_space[2])
		* matrix_sub3x3 - sky * delta_in_view_space[1]);
	eye -= delta_in_world_space;
	target -= delta_in_world_space;
	on_view_moved();
}

void view::pan( float azimuth_delta, float inclination_delta )
{
	glm::vec3 forward = target-eye;
	glm::vec3 side = glm::cross(forward, sky);
	target += glm::rotate(forward, glm::radians(azimuth_delta), sky)
		+ glm::rotate(forward, glm::radians(inclination_delta), side) - 2.0f * forward;
	on_view_moved();
}



void view::on_view_moved()
{
	view_matrix = glm::lookAt(eye, target, sky); 
	view_projection_matrix = projection_matrix * view_matrix;
}

void view::on_window_resized( int width, int height )
{
	window.x = std::max(width, 1);
	window.y = std::max(height, 1);
	window_f.x = static_cast<float>(window.x);
	window_f.y = static_cast<float>(window.y);
	aspect_ratio = window_f.x / window_f.y;
	switch (projection) {
	case orthographic:
		projection_matrix = glm::ortho(left, right_, bottom, top, z_near, z_far);
		break;
	case perspective:
		projection_matrix = glm::perspective(fovy, aspect_ratio, z_near, z_far);
		break;
	case none: break;
	default: assert(false);
	}

	if (none != projection)
		view_projection_matrix = projection_matrix * view_matrix;
}

} // namespace rendering
} // namespace black_label
