#ifndef BLACK_LABEL_RENDERING_PASS_HPP
#define BLACK_LABEL_RENDERING_PASS_HPP

#include <black_label/rendering/assets.hpp>
#include <black_label/rendering/view.hpp>
#include <black_label/rendering/gpu/model.hpp>
#include <black_label/rendering/gpu/framebuffer.hpp>
#include <black_label/rendering/gpu/texture.hpp>
#include <black_label/rendering/light_grid.hpp>
#include <black_label/rendering/program.hpp>
#include <black_label/rendering/screen_aligned_quad.hpp>
#include <black_label/utility/algorithm.hpp>
#include <black_label/world/entities.hpp>

#include <bitset>
#include <chrono>

#include <boost/range/adaptor/indirected.hpp>

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/glm.hpp>

namespace black_label {
namespace rendering {



////////////////////////////////////////////////////////////////////////////////
/// Render Mode
////////////////////////////////////////////////////////////////////////////////
class render_mode {
public:
	enum options : std::size_t {
		statics,
		dynamics,
		screen_aligned_quad,
		materials,
		test_depth,
		SIZE
	};
	std::bitset<SIZE> bitset;

	render_mode& set( std::size_t position, bool value = true )
	{ bitset.set(position, value); return *this; }

	bool operator[]( std::size_t position ) const
	{ return bitset[position]; }
};



////////////////////////////////////////////////////////////////////////////////
/// Basic Pass
////////////////////////////////////////////////////////////////////////////////
class basic_pass
{
public:
	using texture_container = std::vector<std::pair<std::string, std::shared_ptr<gpu::storage_texture>>>;

	basic_pass() {}
	basic_pass(
		std::string name,
		std::shared_ptr<program> program,
		unsigned int clearing_mask,
		unsigned int face_culling_mode,
		render_mode render_mode )
		: name(std::move(name))
		, program{std::move(program)}
		, clearing_mask{clearing_mask}
		, face_culling_mode{face_culling_mode}
		, render_mode(render_mode)
	{}

	template<typename range>
	void set_viewport( const view& view, const range& output_textures ) const {
		int width, height;
		if (boost::empty(output_textures)) {
			width = view.window.x;
			height = view.window.y;
		} else {
			const gpu::storage_texture& first_texture = *std::cbegin(output_textures);
			width = static_cast<int>(view.window.x * first_texture.width);
			height = static_cast<int>(view.window.y * first_texture.height);
		}
		
		set_viewport(width, height);
		program->set_uniform("window_dimensions", width, height);
	}
	void set_viewport( int width, int height ) const;
	void set_blend_mode() const;
	void set_depth_test() const;
	void set_face_culling_mode() const;
	template<typename range>
	bool set_framebuffer( gpu::framebuffer& framebuffer, const range& output_textures ) const {
		if (boost::empty(output_textures)) {
			gpu::framebuffer::unbind();
			return true;
		}

		framebuffer.bind();
		framebuffer.attach(output_textures);
		return framebuffer.is_complete();
	}
	void set_clearing_mask() const;

	static void wait_for_opengl();

	template<typename assets_type, typename range>
	void render( 
		gpu::framebuffer& framebuffer, 
		const assets_type& assets, 
		const view& view,
		const range& output_textures ) const
	{
		auto start_time = std::chrono::high_resolution_clock::now();
		unsigned int texture_unit{0};
		program->use();
		render(framebuffer, assets, view, output_textures, texture_unit);
#ifdef _DEBUG
		wait_for_opengl();
#endif
		render_time = std::chrono::high_resolution_clock::now() - start_time;
	}

	template<typename assets_type, typename callable>
	void render_statics( const assets_type& assets, const view& view, callable render ) const {
		using namespace std;
		using namespace boost::adaptors;

		for (auto entities : assets.statics | map_values) {
			for (auto model_and_matrix : combine(get<assets_type::model_container>(entities), get<assets_type::transformation_range>(entities))) {
				const auto& model = get<0>(model_and_matrix);
				const auto& model_matrix = get<1>(model_and_matrix);

				// Uniforms
				program->set_uniform("normal_matrix", 
					glm::inverseTranspose(glm::mat3(model_matrix)));
				program->set_uniform("model_matrix", 
					model_matrix);
				program->set_uniform("model_view_matrix", 
					view.view_matrix * model_matrix);
				program->set_uniform("model_view_projection_matrix", 
					view.view_projection_matrix * model_matrix);

				render(*model);
			}
		}
	}

	void render_screen_aligned_quad( const view& view ) const {
		static screen_aligned_quad screen_aligned_quad;
		if (black_label::rendering::view::perspective == view.projection)
			screen_aligned_quad.update(view);
		screen_aligned_quad.mesh.render();
	}

	std::string name;
	std::shared_ptr<program> program;
	unsigned int clearing_mask, face_culling_mode;
	render_mode render_mode;
	mutable std::chrono::high_resolution_clock::duration render_time;



protected:
	template<typename assets_type, typename range>
	void render( 
		gpu::framebuffer& framebuffer, 
		const assets_type& assets,
		const view& view,
		const range& output_textures,
		unsigned int& texture_unit ) const
	{
		set_viewport(view, output_textures);
		set_blend_mode();
		set_depth_test();
		set_face_culling_mode();
		if (!set_framebuffer(framebuffer, output_textures)) {
			BOOST_LOG_TRIVIAL(warning) << "The framebuffer is not complete.";
			return;
		}
		set_clearing_mask();
		if (render_mode[render_mode::statics]) {
			if (render_mode[render_mode::materials])
				render_statics(assets, view, [this, texture_unit] ( const auto& model ) mutable { model.render(*program, texture_unit); });
			else
				render_statics(assets, view, [] ( const auto& model ) { model.render(); });
		}
		if (render_mode[render_mode::screen_aligned_quad]) render_screen_aligned_quad(view);
	}
};



////////////////////////////////////////////////////////////////////////////////
/// Pass
////////////////////////////////////////////////////////////////////////////////
class pass : public basic_pass
{
public:
	using basic_pass::texture_container;
	using view_container = std::vector<std::pair<std::string, std::shared_ptr<view>>>;
	using buffer_container = std::vector<std::pair<std::string, std::shared_ptr<gpu::buffer>>>;
	using index_bound_buffer_container = std::vector<std::pair<std::string, std::shared_ptr<gpu::index_bound_buffer>>>;

	pass() : view{nullptr} {}
	pass( 
		std::string name,
		std::shared_ptr<black_label::rendering::program> program, 
		texture_container input_textures,
		texture_container output_textures,
		view_container auxiliary_views,
		buffer_container buffers,
		index_bound_buffer_container index_bound_buffers,
		unsigned int clearing_mask,
		unsigned int post_memory_barrier_mask,
		unsigned int face_culling_mode,
		black_label::rendering::render_mode render_mode,
		const black_label::rendering::view* view,
		const black_label::rendering::view* user_view,
		int preincrement_buffer_counter = 0 ) 
		: basic_pass{
			std::move(name),
			std::move(program), 
			clearing_mask, 
			face_culling_mode,
			render_mode}
		, input_textures(std::move(input_textures))
		, output_textures(std::move(output_textures))
		, auxiliary_views(std::move(auxiliary_views))
		, buffers(std::move(buffers))
		, index_bound_buffers(std::move(index_bound_buffers))
		, post_memory_barrier_mask{post_memory_barrier_mask}
		, view{std::move(view)}
		, user_view{std::move(user_view)}
		, preincrement_buffer_counter{preincrement_buffer_counter}
	{}

	void set_input_textures( unsigned int& texture_unit ) const;
	void set_buffers( unsigned int& shader_storage_binding_point, unsigned int& uniform_binding_point ) const;
	void set_uniforms() const;
	template<typename light_container>
	void set_lights( 
		const light_container& lights, 
		const gpu::buffer& light_buffer, 
		unsigned int& texture_unit, 
		unsigned int& uniform_binding_point ) const
	{
		using namespace std;
		using namespace gpu;

		static vector<float> test_lights;
		test_lights.clear();
		int shadow_map_index{0};
		int light_count{0};

		for (const light& light : lights)
		{
			++light_count;
			test_lights.push_back(light.position.x);
			test_lights.push_back(light.position.y);
			test_lights.push_back(light.position.z);
			test_lights.push_back(light.color.r);
			test_lights.push_back(light.color.g);
			test_lights.push_back(light.color.b);

			if (!light.shadow_map) continue;

			auto name = "shadow_map_" + to_string(shadow_map_index++);
			light.shadow_map.use(*program, name.c_str(), texture_unit);
			program->set_uniform_block("light_block", uniform_binding_point, light_buffer);
		}

		static texture_buffer gpu_lights{usage::stream_draw, format::r32f};

		if (!test_lights.empty())
			gpu_lights.bind_buffer_and_update(test_lights.size(), test_lights.data());
		else
			gpu_lights.bind_buffer_and_update<float>(1);

		gpu_lights.use(*program, "lights", texture_unit);


#ifndef USE_TILED_SHADING
		program->set_uniform("lights_size", light_count);
#endif
	}
	void set_auxiliary_views( unsigned int& uniform_binding_point ) const;
	void set_memory_barrier() const;

	template<typename assets_type>
	void render( gpu::framebuffer& framebuffer, const assets_type& assets ) const {
		using namespace boost::adaptors;
		auto start_time = std::chrono::high_resolution_clock::now();
		unsigned int 
			texture_unit{0}, 
			uniform_binding_point{0}, 
			shader_storage_binding_point{0};
		program->use();
		set_input_textures(texture_unit);
		set_buffers(shader_storage_binding_point, uniform_binding_point);
		set_uniforms();
		set_lights(assets.lights, assets.light_buffer, texture_unit, uniform_binding_point);
		set_auxiliary_views(uniform_binding_point);
		basic_pass::render(
			framebuffer, 
			assets, 
			*view, 
			output_textures | map_values | indirected, 
			texture_unit);
		set_memory_barrier();
#ifdef _DEBUG
		wait_for_opengl();
#endif
		render_time = std::chrono::high_resolution_clock::now() - start_time;
	}

	texture_container input_textures, output_textures;
	view_container auxiliary_views;
	buffer_container buffers;
	index_bound_buffer_container index_bound_buffers;
	unsigned int post_memory_barrier_mask;
	const black_label::rendering::view
		* view,
		* user_view;
	int preincrement_buffer_counter;
};



} // namespace rendering
} // namespace black_label



#endif
