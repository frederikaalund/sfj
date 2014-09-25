#ifndef BLACK_LABEL_RENDERING_SCREEN_ALIGNED_QUAD_HPP
#define BLACK_LABEL_RENDERING_SCREEN_ALIGNED_QUAD_HPP

#include <black_label\rendering\view.hpp>
#include <black_label\rendering\gpu\buffer.hpp>
#include <black_label\rendering\gpu\mesh.hpp>



namespace black_label {
namespace rendering {



////////////////////////////////////////////////////////////////////////////////
/// Screen Aligned Quad
////////////////////////////////////////////////////////////////////////////////
class screen_aligned_quad {
public:

	screen_aligned_quad()
		: view_rays{gpu::target::array, gpu::usage::stream_draw, sizeof(float) * 3 * 4}
	{
		using namespace gpu;
		using namespace gpu::argument;
		mesh = vertices{
				1.0f, 1.0f, 0.0f,
				-1.0f, 1.0f, 0.0f,
				-1.0f, -1.0f, 0.0f,
				1.0f, -1.0f, 0.0f}
			+ indices{0u, 1u, 2u, 2u, 3u, 0u};
	}

	void update( const view& view ) const {
		auto tan_alpha = tan(view.fovy * 0.5);
		auto y = tan_alpha * view.z_far;
		auto x = y * view.aspect_ratio;

		glm::vec3 directions[4] = {
			glm::vec3(x, y, -view.z_far),
			glm::vec3(-x, y, -view.z_far),
			glm::vec3(-x, -y, -view.z_far),
			glm::vec3(x, -y, -view.z_far)
		};

		auto inverse_view_matrix = glm::inverse(glm::mat3(view.view_matrix));

		directions[0] = inverse_view_matrix * directions[0];
		directions[1] = inverse_view_matrix * directions[1];
		directions[2] = inverse_view_matrix * directions[2];
		directions[3] = inverse_view_matrix * directions[3];

		mesh.vertex_array.bind();
		unsigned int index{1u};
		mesh.vertex_array.add_attribute(index, 3, nullptr);
		view_rays.bind_and_update(0, sizeof(float)* 3 * 4, directions);
	}

	gpu::mesh mesh;
	gpu::buffer view_rays;
};



} // namespace rendering
} // namespace black_label



#endif
