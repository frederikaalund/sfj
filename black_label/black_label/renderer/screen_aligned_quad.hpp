#ifndef BLACK_LABEL_RENDERER_SCREEN_ALIGNED_QUAD_HPP
#define BLACK_LABEL_RENDERER_SCREEN_ALIGNED_QUAD_HPP

#include <black_label\renderer\camera.hpp>
#include <black_label\renderer\gpu\buffer.hpp>
#include <black_label\renderer\gpu\mesh.hpp>



namespace black_label {
namespace renderer {



////////////////////////////////////////////////////////////////////////////////
/// Screen Aligned Quad
////////////////////////////////////////////////////////////////////////////////
class screen_aligned_quad {
public:

	screen_aligned_quad()
		: camera_rays{gpu::target::array, gpu::usage::stream_draw, sizeof(float) * 3 * 4}
	{
		using namespace gpu;
		using namespace gpu::argument;

		mesh_ = vertices{
				1.0f, 1.0f, 0.0f,
				-1.0f, 1.0f, 0.0f,
				-1.0f, -1.0f, 0.0f,
				1.0f, -1.0f, 0.0f}
			+ indices{0u, 1u, 2u, 2u, 3u, 0u};
	}

	void update( const camera& camera ) const {
		auto tan_alpha = tan(camera.fovy * 0.5);
		auto x = tan_alpha * camera.z_far;
		auto y = x / camera.aspect_ratio;

		glm::vec3 directions[4] = {
			glm::vec3(x, y, -camera.z_far),
			glm::vec3(-x, y, -camera.z_far),
			glm::vec3(-x, -y, -camera.z_far),
			glm::vec3(x, -y, -camera.z_far)
		};

		auto inverse_view_matrix = glm::inverse(glm::mat3(camera.view_matrix));

		directions[0] = inverse_view_matrix * directions[0];
		directions[1] = inverse_view_matrix * directions[1];
		directions[2] = inverse_view_matrix * directions[2];
		directions[3] = inverse_view_matrix * directions[3];

		mesh_.vertex_array.bind();
		unsigned int index{1u};
		mesh_.vertex_array.add_attribute(index, 3, nullptr);
		camera_rays.bind_and_update(0, sizeof(float)* 3 * 4, directions);
	}

	gpu::mesh mesh_;
	gpu::buffer camera_rays;
};



} // namespace renderer
} // namespace black_label



#endif
