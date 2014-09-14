#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/renderer/assets.hpp>

#include <unordered_set>

#include <GL/glew.h>



using namespace std;



namespace black_label {
namespace renderer {



using namespace gpu;



////////////////////////////////////////////////////////////////////////////////
/// Assets
////////////////////////////////////////////////////////////////////////////////
void assets::update_statics() {
	// Process removed entities
	for (shared_ptr<const entities> entities; removed_statics.try_pop(entities);) {
		// Search for the entities among the existing statics
		auto existing = find_if(statics.cbegin(), statics.cend(), [&entities] ( const auto& value )
			{ return get<0>(value) == entities; });

		if (statics.cend() == existing) continue;
		statics.erase(existing);
	}

	// Process dirty (new or updated) entities
	for (shared_ptr<const entities> entities; dirty_statics.try_pop(entities);) {

		// Search for the entities among the existing statics
		auto existing = find_if(statics.begin(), statics.end(), [&entities] ( const auto& value )
			{ return get<0>(value) == entities; });

		// Associated models
		vector<shared_ptr<model>> associated_models;
		associated_models.reserve(entities->size);
		for (auto entity : *entities)
		{
			auto& model_identifier = entity.model();

			{
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
			dirty_models.push(entity.model());
		}

		// Existing entities
		if (statics.end() != existing) get<1>(*existing) = associated_models;
		// New entities
		else statics.emplace_back(entities, move(associated_models));
	}
}

void assets::update_models() {
	for (model_identifier model; dirty_models.try_pop(model);)
		import_group.run([this, model = std::move(model)] { import_model(std::move(model)); });
}

void assets::update_lights() {
	lights.clear();
	
	for (auto entry : statics)
	{
		const auto& entities = get<0>(entry);
		const auto& associated_models = get<1>(entry);

		for (auto entity : *entities)
		{
			const auto& model = *associated_models[entity.index];
			const auto& model_matrix = entity.transformation();

			for (auto light : model.lights)
			{
				light.position = glm::vec3(model_matrix * glm::vec4(light.position, 1.0f));
				
				if (light_type::spot == light.type)
				{
					light.camera = make_shared<black_label::renderer::camera>(
						light.position, 
						light.position + light.direction, 
						glm::vec3{0.0, 1.0, 0.0},
						4096,
						4096,
						10.0f,
						10000.0f);
					light.shadow_map = make_shared<texture>(target::texture_2d, format::depth32f, filter::nearest, wrap::clamp_to_edge);
					light.shadow_map->bind_and_update(light.camera->window.x, light.camera->window.y);

					

					struct light_uniform_block
					{
						glm::mat4 projection_matrix, view_projection_matrix;
						glm::vec4 wc_direction;
						float radius;
					} light_uniform_block;
					static_assert(148 == sizeof(light_uniform_block), "light_uniform_block must match the OpenGL GLSL layout(140) specification.");
					light_uniform_block.projection_matrix = light.camera->projection_matrix;
					light_uniform_block.view_projection_matrix = light.camera->view_projection_matrix;
					light_uniform_block.wc_direction = glm::vec4(light.camera->forward(), 1.0);
					light_uniform_block.radius = 50.0f;

					light_buffer.bind_and_update(sizeof(light_uniform_block), &light_uniform_block);
				}

				lights.push_back(light);
			}
		}
	}
}

bool assets::upload_models() {
	bool lights_need_update{false};
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

		auto& cpu_model = *std::get<1>(entry);

		*gpu_model = gpu::model{cpu_model, textures};
		if (gpu_model->has_lights()) lights_need_update = true;
	}
	return lights_need_update;
}

void assets::upload_textures() {
	for (textures_to_upload_container::value_type entry; textures_to_upload.try_pop(entry);) 
	{
		auto& texture_identifier = entry.first;

		texture_map::accessor accessor;
		if (!textures.find(accessor, texture_identifier)) assert(false);
		
		auto gpu_texture = accessor->second.lock();

		if (!gpu_texture) {
			textures.erase(accessor);
			continue;
		}

		auto& cpu_texture = *entry.second;

		*gpu_texture = texture{cpu_texture};
	}
}

bool assets::reload_model( path path )
{
	canonical_and_preferred(path, asset_directory);
	
	if (!models.count(path)) return false;

	BOOST_LOG_TRIVIAL(info) << "Reloading model: " << path;
	import_group.run([this, path = std::move(path)] { import_model(std::move(path)); });
	return true;
}

bool assets::reload_texture( path path )
{
	canonical_and_preferred(path, asset_directory);

	if (!textures.count(path)) return false;

	BOOST_LOG_TRIVIAL(info) << "Reloading texture: " << path;
	import_group.run([this, path = std::move(path)] { import_texture(std::move(path)); });
	return true;
}



void assets::import_model( model_identifier path_ )
{
	shared_ptr<model> gpu_model;
	{
		model_map::const_accessor accessor;
		if (!models.find(accessor, path_)) return;
		if (!(gpu_model = accessor->second.lock())) return;
	}

	auto original_path = path_;
	if (!canonical_and_preferred(path_, asset_directory))
	{
		BOOST_LOG_TRIVIAL(warning) << "Missing model " << path_;
		return;
	}


	auto cpu_model = make_shared<cpu::model>(path_, defer_import);

	// No need to load unmodified models
	if (gpu_model->checksum == cpu_model->checksum) return;

	// Import the model
	if (cpu_model->import(path_))
	{
		// Handle textures
		unordered_set<path> texture_paths;
		for (const auto& mesh : cpu_model->meshes)
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
			import_group.run([this, texture_path] { import_texture(std::move(texture_path)); });

		models_to_upload.push(make_tuple(original_path, cpu_model, std::move(gpu_textures)));
	}
}

void assets::import_texture( path path_ )
{
	shared_ptr<texture> gpu_texture;
	{
		texture_map::accessor accessor;
		if (!textures.find(accessor, path_)) return;
		if (!(gpu_texture = accessor->second.lock())) return;
	}

	auto cpu_texture = make_shared<cpu::texture>(path_, defer_import);

	// No need to load unmodified textures
	if (gpu_texture->checksum == cpu_texture->checksum) return;

	// Import the texture
	if (cpu_texture->import(path_))
		textures_to_upload.push({path_, cpu_texture});
}


    
} // namespace renderer
} // namespace black_label
