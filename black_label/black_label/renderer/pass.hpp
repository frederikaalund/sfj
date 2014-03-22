#ifndef BLACK_LABEL_RENDERER_PASS_HPP
#define BLACK_LABEL_RENDERER_PASS_HPP



#include <black_label/renderer/camera.hpp>
#include <black_label/renderer/gpu/texture.hpp>
#include <black_label/renderer/program.hpp>

#include <memory>
#include <string>
#include <utility>
#include <vector>



namespace black_label {
namespace renderer {



class pass
{
public:
	using buffer_container = std::vector<std::pair<std::string, std::shared_ptr<gpu::texture>>>;

	pass() {}
	pass( 
		std::shared_ptr<program> program_, 
		buffer_container input,
		buffer_container output,
		unsigned int clearing_mask,
		bool test_depth,
		bool render_statics,
		bool render_dynamics,
		bool render_screen_aligned_quad,
		std::shared_ptr<black_label::renderer::camera> camera ) 
		: program_{move(program_)}
		, input{move(input)}
		, output{move(output)}
		, clearing_mask{clearing_mask}
		, test_depth{test_depth}
		, render_statics{render_statics}
		, render_dynamics{render_dynamics}
		, render_screen_aligned_quad{render_screen_aligned_quad}
		, camera{std::move(camera)}
	{}
	pass( const pass& ) = default;
	pass( pass&& ) = default;

	std::shared_ptr<program> program_;
	buffer_container input, output;
	unsigned int clearing_mask;
	bool test_depth, render_statics, render_dynamics, render_screen_aligned_quad;
	std::shared_ptr<black_label::renderer::camera> camera;
};



} // namespace renderer
} // namespace black_label



#endif
