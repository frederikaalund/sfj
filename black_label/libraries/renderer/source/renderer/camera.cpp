#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/renderer/camera.hpp>



namespace black_label {
namespace renderer {

void camera::strafe( glm::vec3 delta_in_camera_space )
{
	glm::mat3 matrix_sub3x3(view_matrix);
	glm::vec3 delta_in_world_space(
		glm::vec3(delta_in_camera_space[0], 0.0f, delta_in_camera_space[2])
		* matrix_sub3x3 - sky * delta_in_camera_space[1]);
	eye -= delta_in_world_space;
	target -= delta_in_world_space;
	on_camera_moved();
}

void camera::pan( float azimuth_delta, float inclination_delta )
{
	glm::vec3 forward = target-eye;
	glm::vec3 side = glm::cross(forward, sky);
	target += glm::rotate(forward, glm::radians(azimuth_delta), sky)
		+ glm::rotate(forward, glm::radians(inclination_delta), side) - 2.0f * forward;
	on_camera_moved();
}



void camera::on_camera_moved()
{
	view_matrix = glm::lookAt(eye, target, sky); 
	view_projection_matrix = projection_matrix * view_matrix;
}

void camera::on_window_resized( int width, int height )
{
	window.x = width;
	window.y = (0 < height) ? height : 1;
	window_f.x = static_cast<float>(width);
	window_f.y = static_cast<float>(height);
	aspect_ratio = window_f.x / window_f.y;
	projection_matrix = glm::perspective(fovy, aspect_ratio, z_near, z_far);
	view_projection_matrix = projection_matrix * view_matrix;
}

} // namespace renderer
} // namespace black_label
