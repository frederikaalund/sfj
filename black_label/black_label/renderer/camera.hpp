#ifndef BLACK_LABEL_RENDERER_CAMERA_HPP
#define BLACK_LABEL_RENDERER_CAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>



namespace black_label
{
namespace renderer
{

class camera
{
public:
	camera() {}

	camera( const glm::vec3 eye, const glm::vec3 target, const glm::vec3 up )
		: eye(eye), target(target), up(up) { update_matrix(); }

	void update_matrix() { matrix = glm::lookAt(eye, target, up); }

	void strafe( glm::vec3 delta_in_camera_space )
	{
		glm::mat3 matrix_sub3x3(matrix);
		glm::vec3 delta_in_world_space(
			glm::vec3(delta_in_camera_space[0], 0.0f, delta_in_camera_space[2])
			* matrix_sub3x3 - up*delta_in_camera_space[1]);
		eye -= delta_in_world_space;
		target -= delta_in_world_space;
		update_matrix();
	}

	void pan( float azimuth_delta, float inclination_delta )
	{
		glm::vec3 heading = target-eye;
		glm::vec3 side = glm::cross(heading, up);
		target += glm::rotate(heading, azimuth_delta, up)
			+ glm::rotate(heading, inclination_delta, side) - 2.0f*heading;
		update_matrix();
	}

	glm::mat4 matrix;
	glm::vec3 eye, target, up;
protected:
private:
};

} // namespace renderer
} // namespace black_label



#endif