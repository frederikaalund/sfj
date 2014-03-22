// Version 0.1
#ifndef BLACK_LABEL_RENDERER_RENDERER_HPP
#define BLACK_LABEL_RENDERER_RENDERER_HPP



//#define USE_TILED_SHADING
#define USE_TEXTURE_BUFFER



#include <black_label/container.hpp>
#include <black_label/renderer/camera.hpp>
#include <black_label/renderer/light.hpp>
#include <black_label/renderer/light_grid.hpp>
#include <black_label/renderer/pipeline.hpp>
#include <black_label/renderer/program.hpp>
#include <black_label/renderer/gpu/model.hpp>
#include <black_label/shared_library/utility.hpp>
#include <black_label/utility/threading_building_blocks/path.hpp>
#include <black_label/utility/warning_suppression.hpp>
#include <black_label/world.hpp>

#include <algorithm>
#include <memory>
#include <unordered_map>
#include <vector>

#include <boost/container/stable_vector.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/log/trivial.hpp>
#include <boost/program_options.hpp>

#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_map.h>



namespace black_label {
namespace renderer {



class BLACK_LABEL_SHARED_LIBRARY renderer
{
MSVC_PUSH_WARNINGS(4251)
private:
	struct glew_setup { glew_setup(); } glew_setup;
MSVC_POP_WARNINGS()

	class locked_statics_guard
	{
	public:
		using statics_type = std::vector<std::pair<std::shared_ptr<const world::entities>, std::vector<std::shared_ptr<gpu::model>>>>;

		friend void swap( locked_statics_guard& lhs, locked_statics_guard& rhs )
		{ std::swap(lhs.locked_statics_, rhs.locked_statics_); }

		locked_statics_guard() : locked_statics_{nullptr} {}
		locked_statics_guard( statics_type* locked_statics_ )
			: locked_statics_{locked_statics_} {}
		locked_statics_guard( const locked_statics_guard& other ) = delete;
		locked_statics_guard( locked_statics_guard&& other ) 
			: locked_statics_guard{} { swap(*this, other); }
		~locked_statics_guard()
		{ if (locked_statics_) locked_statics_->clear(); }
		locked_statics_guard& operator=( locked_statics_guard rhs )
		{ swap(*this, rhs); return *this; }

		statics_type* locked_statics_;
	};



public:
	static const path default_shader_directory;
	static const path default_asset_directory;



	renderer( 
		std::shared_ptr<black_label::renderer::camera> camera_,
		path shader_directory = default_shader_directory, 
		path asset_directory = default_asset_directory );
	renderer( const renderer& ) = delete; // Possible, but do you really want to?
	~renderer();

	void update_statics();
	void optimize_for_rendering( world::group& group );
	locked_statics_guard lock_statics();
	void render_frame();
	void reload_asset( path path )
	{
		if (reload_model(path)) return;
		if (reload_texture(path)) return;
		if (reload_shader(path)) return;
	}
	bool reload_model( path path );
	bool reload_texture( path path );
	bool reload_shader( path path );



	void add_statics( std::shared_ptr<const world::entities> entities )
	{ dirty_statics.push(entities); }
	void add_statics( world::group group )
	{ for (auto entities : group) add_statics(entities); }

	
	void on_window_resized( int width, int height );



	path shader_directory;
	path asset_directory;



	std::shared_ptr<black_label::renderer::camera> camera;

private:

	void construct_screen_aligned_quad();
	void update_camera_ray_buffer();
	void render_pass( const pass& pass ) const;



MSVC_PUSH_WARNINGS(4251)

	using dirty_entities_container = tbb::concurrent_queue<std::weak_ptr<const world::entities>>;
	using entities_and_models = std::pair<std::weak_ptr<const world::entities>, std::vector<std::shared_ptr<gpu::model>>>;
	using entities_container = boost::container::stable_vector<entities_and_models>;

	using model_identifier = world::entities::model_type;
	using dirty_model_container = tbb::concurrent_queue<model_identifier>;
	using model_container = std::unordered_map<model_identifier, std::weak_ptr<gpu::model>>;
	using models_to_upload_container = tbb::concurrent_queue<std::pair<model_identifier, std::shared_ptr<cpu::model>>>;

	

	using light_container = light_grid::light_container;
	using textures_to_upload_container = tbb::concurrent_queue<std::pair<gpu::texture_identifier, std::shared_ptr<cpu::texture>>>;


	template<typename callable>
	void render_group( const locked_statics_guard::statics_type& group, callable render_function ) const
	{
		for (auto entry : group)
		{
			const auto& entities = entry.first;
			const auto& associated_models = entry.second;

			for (auto entity : *entities)
			{
				const auto& model = *associated_models[entity.index];
				const auto& model_matrix = entity.transformation();
				render_function(model, model_matrix);
			}
		}
	}

	void update_lights();
	bool import_model( path path, gpu::model& gpu_model );
	void import_model( model_identifier model );
	void import_texture( path path );
	
	void render_shadow_map();
	void render_geometry_buffer();
	void render_ambient_occlusion();
	void render_ambient_occlusion_blur();
	void render_lighting();
	void render_bloom_blur();
	void render_tonemapping();
	


	dirty_model_container dirty_models;
	dirty_entities_container dirty_statics, dirty_dynamic_entities;

	model_container models;
	models_to_upload_container models_to_upload;
	entities_container statics, sorted_dynamics;
	locked_statics_guard::statics_type locked_statics;
	gpu::texture_container textures;
	textures_to_upload_container textures_to_upload;

	light_container lights;
	light_grid light_grid;
	

	unsigned int framebuffer;
	gpu::texture main_render, depths, shadow_map, wc_normals, albedos, 
		bloom1, bloom2, random_texture, ambient_occlusion_texture;
	gpu::texture lut_texture;
	gpu::texture_buffer gpu_lights;



	float ambient_occlusion_resolution_multiplier, shadow_map_resolution_multiplier;

	gpu::mesh screen_aligned_quad;
	gpu::buffer camera_ray_buffer;

	mutable glm::mat4 light_view_projection_matrix;

	pipeline pipeline;
	int next_shadow_map_id;

	std::vector<glm::vec2> poisson_disc;
	gpu::buffer light_buffer;
	

MSVC_POP_WARNINGS()
};

} // namespace renderer
} // namespace black_label



#endif
