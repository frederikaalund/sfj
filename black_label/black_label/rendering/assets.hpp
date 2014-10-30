#ifndef BLACK_LABEL_RENDERING_ASSETS_HPP
#define BLACK_LABEL_RENDERING_ASSETS_HPP

#include <black_label/rendering/cpu/model.hpp>
#include <black_label/rendering/gpu/model.hpp>
#include <black_label/utility/threading_building_blocks/path.hpp>

#include <unordered_map>
#include <unordered_set>
#include <tuple>
#include <type_traits>

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
	typename model_file_iterator, 
	typename transformation_iterator>
class assets {
private:
	using model_file_type = typename std::iterator_traits<model_file_iterator>::value_type;
	static_assert(std::is_convertible<model_file_type*, path*>::value, 
		"The model_file_iterator must reference a type which can be converted to a path.");

public:
	using model_file_range = boost::iterator_range<model_file_iterator>;
	using transformation_range = boost::iterator_range<transformation_iterator>;
	using model_container = std::vector<std::shared_ptr<gpu::model>>;
	using external_entities = std::tuple<
		id_type,
		model_file_range, 
		transformation_range>;
	using entities = std::tuple<
		model_file_range, 
		transformation_range, 
		model_container>;
	using entities_container = std::unordered_map<id_type, entities>;

	using model_map = concurrent_resource_map<gpu::model>;

	using dirty_entities_container = tbb::concurrent_queue<external_entities>;
	using dirty_model_file_container = tbb::concurrent_queue<path>;
	using missing_model_file_container = dirty_model_file_container;

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
	dirty_model_file_container dirty_model_files;
	// Emptied by calling import_missing_models
	missing_model_file_container missing_model_files;

	// Emptied by calling update
	light_container lights, shadow_casting_lights;
	// N/A
	gpu::buffer light_buffer;

	// Not thread-safe. Do not change while member functions are running
	// except for import_task (which creates a local copy).
	path asset_directory;
	// N/A
	tbb::task_group import_group;



	assets( path asset_directory ) 
		: asset_directory(std::move(asset_directory)) 
		, light_buffer{gpu::target::uniform_buffer, gpu::usage::dynamic_draw, 148}
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
			auto& model_files = get<model_file_range>(entities);
			vector<shared_ptr<model>> associated_models;
			associated_models.reserve(model_files.size());
			for (auto file : model_files) 
			{
				file.make_preferred();
				{
					model_map::accessor accessor;
					auto new_model = make_shared<model>();

					// Attempt to insert new_model
					models.insert(accessor, {file, new_model});
					// The returned model can be either the new_model or an existing model
					auto model = accessor->second.lock();
					// The model has expired. Thus the insertion must have returned an existing model...
					if (!model)
						// ...which is replaced.
						accessor->second = model = move(new_model);

					associated_models.emplace_back(move(model));
				}
			
				// Mark the entity's model as dirty
				dirty_model_files.push(std::move(file));
			}

			// Existing entities
			if (statics.end() != existing) get<model_container>(existing->second) = move(associated_models);
			// New entities
			else statics.emplace(std::piecewise_construct, forward_as_tuple(get<id_type>(entities)), forward_as_tuple(get<model_file_range>(entities), get<transformation_range>(entities), move(associated_models)));
		}
	}
	// Thread-safe; immediate (enqueues a parallel task and returns)
	void update_models() {
		for (path file; dirty_model_files.try_pop(file);)
			import_group.run([this, file = std::move(file)] { import_model(std::move(file), asset_directory); });
	}
	// Thread-safe; immediate (enqueues a parallel task and returns)
	void import_missing_models() {
		// Make a local copy of the currently missing model files...
		std::vector<path> local_missing_model_files;
		for (path file; missing_model_files.try_pop(file);)
			local_missing_model_files.emplace_back(file);

		// ...to avoid an infinite loop here (as the models might still be missing and
		// continuously added to missing_model_files).
		for (auto file : local_missing_model_files)
			import_group.run([this, file = std::move(file)] { import_model(std::move(file), asset_directory); });
	}
	// Must be called by an OpenGL thread
	void upload_models() {
		bool static_lights_need_update{false};
		for (models_to_upload_container::value_type entry; models_to_upload.try_pop(entry);)
		{
			auto& file = std::get<0>(entry);

			model_map::accessor accessor;
			if (!models.find(accessor, file)) assert(false);

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
			auto& file = entry.first;

			texture_map::accessor accessor;
			if (!textures.find(accessor, file)) assert(false);
		
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
	bool reload_model( path file ) {
		if (!try_canonical_and_preferred(file, asset_directory))
			return false;
	
		if (!models.count(file)) return false;

		BOOST_LOG_TRIVIAL(info) << "Reloading model: " << file;
		import_group.run([this, file = std::move(file)] { import_model(std::move(file), asset_directory); });
		return true;
	}
	// Thread-safe; immediate (enqueues a parallel task and returns)
	bool reload_texture( path file ) {
		if (!try_canonical_and_preferred(file, asset_directory))
			return false;

		if (!textures.count(file)) return false;

		BOOST_LOG_TRIVIAL(info) << "Reloading texture: " << file;
		import_group.run([this, file = std::move(file)] { import_texture(std::move(file)); });
		return true;
	}
	// Thread-safe; immediate (enqueues a parallel task and returns)
	bool reload( const path& file ) {
		return reload_model(file) || reload_texture(file);
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
	using models_to_upload_container = tbb::concurrent_queue<std::tuple<path, cpu::model, std::vector<std::shared_ptr<gpu::texture>>>>;
	using textures_to_upload_container = tbb::concurrent_queue<std::pair<path, cpu::texture>>;

	// Emptied by calling update
	models_to_upload_container models_to_upload;
	// Emptied by calling update
	textures_to_upload_container textures_to_upload;



	// Thread-safe; blocking
	template<typename resource>
	bool try_get( const concurrent_resource_map<resource>& map, const path& file, std::shared_ptr<resource>& resource_ ) {
		concurrent_resource_map<resource>::const_accessor accessor;
		if (!map.find(accessor, file) || !(resource_ = accessor->second.lock()))
			return false;
		return true;
	}

	// Thread-safe; blocking
	void import_model( path file, path directory ) {
		using namespace std;
		using namespace gpu;

		system::error_code error_code;
		auto canonical_file = canonical_and_preferred(file, directory, error_code);
		if (error_code) {
			missing_model_files.emplace(file);
			BOOST_LOG_TRIVIAL(warning) << "Missing model " << file;
			return;
		}

		// Attempt to get the GPU model associated with the file. This may fail if the model
		// is deleted before this function is called.
		shared_ptr<model> gpu_model;
		if (!try_get(models, file, gpu_model)) return;


		cpu::model cpu_model{canonical_file, defer_import};

		// No need to load unmodified models
		if (gpu_model->checksum == cpu_model.checksum) return;

		// Import the model
		if (!cpu_model.import(canonical_file)) return;

		// Handle textures
		unordered_set<path> texture_files;
		for (const auto& mesh : cpu_model.meshes)
		{
			if (!mesh.material.diffuse_texture.empty())
				texture_files.emplace(mesh.material.diffuse_texture);
			if (!mesh.material.specular_texture.empty())
				texture_files.emplace(mesh.material.specular_texture);
		}

		vector<shared_ptr<texture>> gpu_textures(texture_files.size());
		for (const auto& texture_file : texture_files)
		{
			texture_map::accessor accessor;
			auto new_texture = make_shared<texture>();

			// Attempt to insert new_texture
			textures.insert(accessor, {texture_file, new_texture});
			// The returned texture can be either the new_texture or an existing texture
			auto texture = accessor->second.lock();
			// The texture has expired. Thus the insertion must have returned an existing texture...
			if (!texture)
				// ...which is replaced.
				accessor->second = texture = move(new_texture);

			gpu_textures.emplace_back(move(texture));
		}

		for (const auto& texture_file : texture_files) 
			import_group.run([this, texture_file] { import_texture(move(texture_file)); });

		models_to_upload.push(make_tuple(move(file), move(cpu_model), move(gpu_textures)));
	}
	// Thread-safe; blocking
	void import_texture( path file ) {
		using namespace std;
		using namespace gpu;

		shared_ptr<texture> gpu_texture;
		if (!try_get(textures, file, gpu_texture)) return;

		cpu::texture cpu_texture{file, defer_import};

		// No need to load unmodified textures
		if (gpu_texture->checksum == cpu_texture.checksum) return;

		// Import the texture
		if (cpu_texture.import(file))
			textures_to_upload.push({move(file), move(cpu_texture)});
	}
};



} // namespace rendering
} // namespace black_label



#endif
