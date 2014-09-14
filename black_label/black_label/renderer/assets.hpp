#ifndef BLACK_LABEL_RENDERER_ASSETS_HPP
#define BLACK_LABEL_RENDERER_ASSETS_HPP



#include <black_label/renderer/cpu/model.hpp>
#include <black_label/renderer/gpu/model.hpp>
#include <black_label/utility/threading_building_blocks/path.hpp>
#include <black_label/world.hpp>

#include <unordered_map>
#include <tuple>

#include <boost/container/stable_vector.hpp>

#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_hash_map.h>
#include <tbb/task_group.h>



namespace black_label {
namespace renderer {



////////////////////////////////////////////////////////////////////////////////
/// Assets
////////////////////////////////////////////////////////////////////////////////
class assets {
public:

	using entities = world::entities;
	using model_identifier = entities::model_type;

	using entities_and_models = std::tuple<
		std::shared_ptr<const entities>, 
		std::vector<std::shared_ptr<gpu::model>>>;

	using container = std::vector<entities_and_models>;
	using model_map = tbb::concurrent_hash_map<model_identifier, std::weak_ptr<gpu::model>>;

	using light_container = gpu::model::light_container;

	using dirty_entities_container = tbb::concurrent_queue<std::shared_ptr<const entities>>;
	using dirty_model_container = tbb::concurrent_queue<model_identifier>;



	// Emptied by calling remove_statics
	container statics;
	// Never emptied
	model_map models;
	// Never emptied
	gpu::texture_map textures;
	// Emptied by calling update
	light_container lights;
	gpu::buffer light_buffer;

	// Emptied by calling update
	dirty_entities_container dirty_statics, removed_statics;
	// Emptied by calling update
	dirty_model_container dirty_models;

	path asset_directory;
	tbb::task_group import_group;



	assets( path asset_directory ) 
		: asset_directory(std::move(asset_directory))
		, light_buffer{gpu::target::uniform_buffer, gpu::usage::dynamic_draw, 128}
	{}
	assets( const assets& other ) = delete;
	~assets() {
		import_group.cancel();
		import_group.wait();
	}

	// Not thread-safe
	void update_statics();
	// Thread-safe; immediate (enqueues a parallel task and returns)
	void update_models();
	// Not thread-safe
	void update_lights();
	// Must be called by an OpenGL thread
	bool upload_models();
	// Must be called by an OpenGL thread
	void upload_textures();
	// Not thread-safe; must be called by an OpenGL thread
	void update() {
		update_statics();
		update_models();
		upload_textures();
		auto lights_need_update = upload_models();
		if (lights_need_update) update_lights();
	}
	// Thread-safe; immediate (enqueues a parallel task and returns)
	bool reload_model( path path );
	// Thread-safe; immediate (enqueues a parallel task and returns)
	bool reload_texture( path path );	

	// Thread-safe; immediate (registers work and awaits an update call)
	void add_statics( std::shared_ptr<const entities> entities )
	{ dirty_statics.push(std::move(entities)); }
	// Thread-safe; immediate (registers work and awaits an update call)
	void add_statics( world::const_group group )
	{ for (auto& entities : group) add_statics(std::move(entities)); }

	// Thread-safe; immediate (registers work and awaits an update call)
	void remove_statics( std::shared_ptr<const entities> entities )
	{ removed_statics.push(std::move(entities)); }
	// Thread-safe; immediate (registers work and awaits an update call)
	void remove_statics( world::const_group group )
	{ for (auto& entities : group) remove_statics(std::move(entities)); }



private:
	using models_to_upload_container = tbb::concurrent_queue<std::tuple<model_identifier, std::shared_ptr<cpu::model>, std::vector<std::shared_ptr<gpu::texture>>>>;
	using textures_to_upload_container = tbb::concurrent_queue<std::pair<gpu::texture_identifier, std::shared_ptr<cpu::texture>>>;

	// Emptied by calling update
	models_to_upload_container models_to_upload;
	// Emptied by calling update
	textures_to_upload_container textures_to_upload;



	// Thread-safe; blocking
	void import_model( model_identifier model );
	// Thread-safe; blocking
	void import_texture( path path );
};



} // namespace renderer
} // namespace black_label



#endif
