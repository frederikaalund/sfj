#ifndef BLACK_LABEL_RENDERING_ASSETS_HPP
#define BLACK_LABEL_RENDERING_ASSETS_HPP

#include <black_label/rendering/cpu/model.hpp>
#include <black_label/rendering/gpu/model.hpp>
#include <black_label/utility/threading_building_blocks/path.hpp>

#include <unordered_map>
#include <unordered_set>
#include <tuple>

#include <boost/range/adaptor/map.hpp>
#include <boost/range/combine.hpp>
#include <boost/range/iterator_range_core.hpp>

#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_unordered_map.h>
#include <tbb/task_group.h>



namespace black_label {
namespace rendering {



////////////////////////////////////////////////////////////////////////////////
/// Assets
////////////////////////////////////////////////////////////////////////////////
template<
	typename id_type,
	typename model_iterator, 
	typename transformation_iterator>
class assets {
public:
	using model_range = boost::iterator_range<model_iterator>;
	using transformation_range = boost::iterator_range<transformation_iterator>;
	using model_container = std::vector<std::shared_ptr<gpu::model>>;
	using external_entities = std::tuple<
		id_type,
		model_range, 
		transformation_range>;
	using entities = std::tuple<
		model_range, 
		transformation_range, 
		model_container>;
	using entities_container = std::unordered_map<id_type, entities>;

	using model_identifier = typename std::iterator_traits<model_iterator>::value_type;
	using model_map = tbb::concurrent_hash_map<model_identifier, std::weak_ptr<gpu::model>>;

	using dirty_entities_container = tbb::concurrent_queue<external_entities>;
	using dirty_model_container = tbb::concurrent_queue<model_identifier>;

	using light_container = std::vector<std::reference_wrapper<light>>;



	// Emptied by calling remove_statics
	entities_container statics;
	// Never emptied
	model_map models;
	// Never emptied
	gpu::texture_map textures;

	// Emptied by calling update
	dirty_entities_container dirty_statics, removed_statics;
	// Emptied by calling update
	dirty_model_container dirty_models;

	// Emptied by calling update
	light_container lights, shadow_casting_lights;
	// N/A
	gpu::buffer light_buffer;

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
	void update_statics() {
		using namespace std;
		using namespace gpu;

		// Process removed entities
		bool static_lights_need_update{false};
		for (external_entities entities; removed_statics.try_pop(entities);) {
			statics.erase(get<id_type>(entities));
			static_lights_need_update = true;
		}
		if (static_lights_need_update) update_static_lights();

		// Process dirty (new or updated) entities
		for (external_entities entities; dirty_statics.try_pop(entities);) {

			// Search for the entities among the existing statics
			auto existing = statics.find(get<id_type>(entities));

			// Associated models
			vector<shared_ptr<model>> associated_models;
			auto& model_range_ = get<model_range>(entities);
			associated_models.reserve(model_range_.size());
			for (const auto& model_identifier : model_range_) 
			{
				{
					using model_map = decltype(this->models);
					model_map::accessor accessor;
					auto new_model = make_shared<model>();

					// Attempt to insert new_model
					models.insert(accessor, {model_identifier, new_model});
					// The returned model can be either the new_model or an existing model
					auto model = accessor->second.lock();
					// The model has expired. Thus the insertion must have returned an existing model...
					if (!model)
						// ...which is replaced.
						accessor->second = model = move(new_model);

					associated_models.emplace_back(move(model));
				}
			
				// Mark the entity's model as dirty
				dirty_models.push(model_identifier);
			}

			// Existing entities
			if (statics.end() != existing) get<model_container>(existing->second) = move(associated_models);
			// New entities
			else statics.emplace(std::piecewise_construct, forward_as_tuple(get<id_type>(entities)), forward_as_tuple(get<model_range>(entities), get<transformation_range>(entities), move(associated_models)));
		}
	}
	// Thread-safe; immediate (enqueues a parallel task and returns)
	void update_models() {
		for (model_identifier model; dirty_models.try_pop(model);)
			import_group.run([this, model = std::move(model)] { import_model(std::move(model)); });
	}
	// Must be called by an OpenGL thread
	void upload_models() {
		bool static_lights_need_update{false};
		for (models_to_upload_container::value_type entry; models_to_upload.try_pop(entry);)
		{
			auto& model_identifier = std::get<0>(entry);

			model_map::accessor accessor;
			if (!models.find(accessor, model_identifier)) assert(false);

			auto gpu_model = accessor->second.lock();

			if (!gpu_model) {
				models.erase(accessor);
				continue;
			}

			auto& cpu_model = std::get<1>(entry);

			*gpu_model = gpu::model{std::move(cpu_model), textures};

			if (gpu_model->has_lights()) static_lights_need_update = true;
		}

		if (static_lights_need_update) update_static_lights();
	}
	// Not thread-safe
	void update_static_lights() {
		using namespace gpu;
		using namespace std;
		using namespace boost::adaptors;

		lights.clear();
		shadow_casting_lights.clear();

		for (auto entities : statics | map_values) {
			for (auto model_and_matrix : combine(get<model_container>(entities), get<transformation_range>(entities))) {
				const auto& model = get<0>(model_and_matrix);
				const auto& model_matrix = get<1>(model_and_matrix);

				for (light& light : model->lights) {
					light.position = glm::vec3(model_matrix * glm::vec4(light.position, 1.0f));
					lights.push_back(light);
				
					if (light_type::spot == light.type) {
						light.view = view{
							light.position, 
							light.position + light.direction, 
							glm::vec3{0.0, 1.0, 0.0},
							800,
							800,
							10.0f,
							10000.0f};
						light.shadow_map = storage_texture{target::texture_2d, format::depth32f, filter::nearest, wrap::clamp_to_edge, 1.0, 1.0};
						light.shadow_map.bind_and_update(light.view.window.x, light.view.window.y);

						struct light_uniform_block {
							glm::mat4 projection_matrix, view_projection_matrix;
							glm::vec4 wc_direction;
							float radius;
						} light_uniform_block;
						static_assert(148 == sizeof(light_uniform_block), "light_uniform_block must match the OpenGL GLSL layout(140) specification.");
						light_uniform_block.projection_matrix = light.view.projection_matrix;
						light_uniform_block.view_projection_matrix = light.view.view_projection_matrix;
						light_uniform_block.wc_direction = glm::vec4(light.view.forward(), 1.0);
						light_uniform_block.radius = 50.0f;

						light_buffer.bind_and_update(sizeof(light_uniform_block), &light_uniform_block);

						shadow_casting_lights.push_back(light);
					}
				}
			}
		}
	}
	// Must be called by an OpenGL thread
	void upload_textures() {
		using namespace gpu;

		for (textures_to_upload_container::value_type entry; textures_to_upload.try_pop(entry);) {
			auto& texture_identifier = entry.first;

			texture_map::accessor accessor;
			if (!textures.find(accessor, texture_identifier)) assert(false);
		
			auto gpu_texture = accessor->second.lock();

			if (!gpu_texture) {
				textures.erase(accessor);
				continue;
			}

			auto& cpu_texture = entry.second;

			*gpu_texture = texture{std::move(cpu_texture)};
		}
	}
	// Not thread-safe; must be called by an OpenGL thread
	void update() {
		update_statics();
		update_models();
		upload_textures();
		upload_models();
	}
	// Thread-safe; immediate (enqueues a parallel task and returns)
	bool reload_model( path path ) {
		canonical_and_preferred(path, asset_directory);
	
		if (!models.count(path)) return false;

		BOOST_LOG_TRIVIAL(info) << "Reloading model: " << path;
		import_group.run([this, path = std::move(path)] { import_model(std::move(path)); });
		return true;
	}
	// Thread-safe; immediate (enqueues a parallel task and returns)
	bool reload_texture( path path ) {
		canonical_and_preferred(path, asset_directory);

		if (!textures.count(path)) return false;

		BOOST_LOG_TRIVIAL(info) << "Reloading texture: " << path;
		import_group.run([this, path = std::move(path)] { import_texture(std::move(path)); });
		return true;
	}

	// Thread-safe; immediate (registers work and awaits an update call)
	void add_statics( const external_entities& entities )
	{ dirty_statics.push(entities); }
	// Thread-safe; immediate (registers work and awaits an update call)
	template<typename iterator>
	void add_statics( iterator first, iterator last )
	{ std::for_each(first, last, [this] ( const auto& entities ) { add_statics(entities); }); }

	// Thread-safe; immediate (registers work and awaits an update call)
	void remove_statics( const external_entities& entities )
	{ removed_statics.push(entities); }
	// Thread-safe; immediate (registers work and awaits an update call)
	template<typename iterator>
	void remove_statics( iterator first, iterator last )
	{ std::for_each(first, last, [this] ( const auto& entities ) { remove_statics(entities); }); }



private:
	using models_to_upload_container = tbb::concurrent_queue<std::tuple<model_identifier, cpu::model, std::vector<std::shared_ptr<gpu::texture>>>>;
	using textures_to_upload_container = tbb::concurrent_queue<std::pair<gpu::texture_identifier, cpu::texture>>;

	// Emptied by calling update
	models_to_upload_container models_to_upload;
	// Emptied by calling update
	textures_to_upload_container textures_to_upload;



	// Thread-safe; blocking
	void import_model( model_identifier path ) {
		using namespace std;
		using namespace gpu;

		shared_ptr<model> gpu_model;
		{
			model_map::const_accessor accessor;
			if (!models.find(accessor, path)) return;
			if (!(gpu_model = accessor->second.lock())) return;
		}

		auto original_path = path;
		if (!canonical_and_preferred(path, asset_directory))
		{
			BOOST_LOG_TRIVIAL(warning) << "Missing model " << path;
			return;
		}


		cpu::model cpu_model{path, defer_import};

		// No need to load unmodified models
		if (gpu_model->checksum == cpu_model.checksum) return;

		// Import the model
		if (cpu_model.import(path))
		{
			// Handle textures
			unordered_set<black_label::path> texture_paths;
			for (const auto& mesh : cpu_model.meshes)
			{
				if (!mesh.material.diffuse_texture.empty())
					texture_paths.emplace(mesh.material.diffuse_texture);
				if (!mesh.material.specular_texture.empty())
					texture_paths.emplace(mesh.material.specular_texture);
			}

			vector<shared_ptr<texture>> gpu_textures(texture_paths.size());
			for (const auto& texture_path : texture_paths)
			{
				texture_map::accessor accessor;
				auto new_texture = make_shared<texture>();

				// Attempt to insert new_texture
				textures.insert(accessor, {texture_path, new_texture});
				// The returned texture can be either the new_texture or an existing texture
				auto texture = accessor->second.lock();
				// The texture has expired. Thus the insertion must have returned an existing texture...
				if (!texture)
					// ...which is replaced.
					accessor->second = texture = move(new_texture);

				gpu_textures.emplace_back(move(texture));
			}

			for (const auto& texture_path : texture_paths) 
				import_group.run([this, texture_path] { import_texture(move(texture_path)); });

			models_to_upload.push(make_tuple(move(original_path), move(cpu_model), move(gpu_textures)));
		}
	}
	// Thread-safe; blocking
	void import_texture( path path ) {
		using namespace std;
		using namespace gpu;

		shared_ptr<texture> gpu_texture;
		{
			texture_map::accessor accessor;
			if (!textures.find(accessor, path)) return;
			if (!(gpu_texture = accessor->second.lock())) return;
		}

		cpu::texture cpu_texture{path, defer_import};

		// No need to load unmodified textures
		if (gpu_texture->checksum == cpu_texture.checksum) return;

		// Import the texture
		if (cpu_texture.import(path))
			textures_to_upload.push({move(path), move(cpu_texture)});
	}
};



} // namespace rendering
} // namespace black_label



#endif
