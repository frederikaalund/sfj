#ifndef BLACK_LABEL_RENDERER_PASS_HPP
#define BLACK_LABEL_RENDERER_PASS_HPP



#include <black_label/renderer/assets.hpp>
#include <black_label/renderer/camera.hpp>
#include <black_label/renderer/gpu/model.hpp>
#include <black_label/renderer/gpu/texture.hpp>
#include <black_label/renderer/gpu/framebuffer.hpp>
#include <black_label/renderer/light_grid.hpp>
#include <black_label/renderer/program.hpp>
#include <black_label/world/entities.hpp>

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/container/stable_vector.hpp>



namespace black_label {
namespace renderer {



class pass
{
public:
	using texture_container = std::vector<std::pair<std::string, std::shared_ptr<gpu::texture>>>;
	using buffer_container = std::vector<std::pair<std::string, std::shared_ptr<gpu::buffer>>>;

	pass() {}
	pass( 
		std::shared_ptr<program> program_, 
		texture_container input_textures,
		texture_container output_textures,
		buffer_container buffers,
		unsigned int clearing_mask,
		unsigned int post_memory_barrier_mask,
		unsigned int face_culling_mode,
		bool test_depth,
		bool render_statics,
		bool render_dynamics,
		bool render_screen_aligned_quad,
		std::shared_ptr<black_label::renderer::camera> camera ) 
		: program_{move(program_)}
		, input_textures(move(input_textures))
		, output_textures(move(output_textures))
		, buffers(move(buffers))
		, clearing_mask{clearing_mask}
		, post_memory_barrier_mask{post_memory_barrier_mask}
		, face_culling_mode{face_culling_mode}
		, test_depth{test_depth}
		, render_statics{render_statics}
		, render_dynamics{render_dynamics}
		, render_screen_aligned_quad{render_screen_aligned_quad}
		, camera{std::move(camera)}
	{}

	void pass::render( 
		const gpu::framebuffer& framebuffer, 
		const assets& assets ) const;

	std::shared_ptr<program> program_;
	texture_container input_textures, output_textures;
	buffer_container buffers;
	unsigned int clearing_mask, post_memory_barrier_mask, face_culling_mode;
	bool test_depth, render_statics, render_dynamics, render_screen_aligned_quad;
	std::shared_ptr<black_label::renderer::camera> camera;
};



} // namespace renderer
} // namespace black_label



#endif
