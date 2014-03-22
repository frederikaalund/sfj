#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/renderer.hpp>

#include <black_label/renderer/cpu/model.hpp>

#include <array>
#include <cstdlib>
#include <cstdint>
#include <utility>
#include <fstream>
#include <iostream>
#include <random>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/date_time.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/log/trivial.hpp>
#include <boost/math/constants/constants.hpp>

#include <tbb/task.h>

#include <GL/glew.h>
#ifndef APPLE
#include <GL/gl.h>
#else
#include <OpenGL/gl.h>
#endif

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#define OCIO_BUILD_STATIC
#include <OpenColorIO/OpenColorIO.h>
namespace ocio = OCIO_NAMESPACE;




using namespace std;
using namespace tbb;



namespace black_label {
namespace renderer {

using namespace gpu;
using namespace utility;

		

renderer::glew_setup::glew_setup()
{
	glewExperimental = GL_TRUE;
	GLenum glew_error = glewInit();
	glGetError(); // Unfortunately, GLEW produces an error for 3.2 contexts. Calling glGetError() dismisses it.
  
	if (GLEW_OK != glew_error)
		throw runtime_error("Glew failed to initialize.");

	if (!GLEW_VERSION_3_3)
		throw runtime_error("Requires OpenGL version 3.3 or above.");

	GLint max_draw_buffers;
	glGetIntegerv(GL_MAX_DRAW_BUFFERS, &max_draw_buffers);
	if (3 > max_draw_buffers)
		throw runtime_error("Requires at least 3 draw buffers.");
}



const path renderer::default_shader_directory;
const path renderer::default_asset_directory;

// Implemented via relaxation dart throwing
std::vector<glm::vec2> generate_poisson_disc( int samples )
{
	random_device random_device;
	mt19937 engine{random_device()};
	uniform_real_distribution<float> distribution{-1.0f, 1.0f};
	float distance_threshold{0.5};
	int rejects{0};
	static const int rejects_before_relaxation{10};
	static const float relaxation_coefficient{0.9f};
	std::vector<glm::vec2> accepted_samples;

	while (accepted_samples.size() < samples)
	{
		// Generate sample in unit disc
		glm::vec2 sample;
		do 
		{
			sample.x = distribution(engine);
			sample.y = distribution(engine);
		} while (glm::length(sample) > 1.0f);

		// Reject sample if it is too close to the accepted samples
		bool reject{false};
		for (auto accepted_sample : accepted_samples)
			if (glm::distance(sample, accepted_sample) < distance_threshold)
			{ reject = true; break; }
		if (reject) 
		{
			if (++rejects >= rejects_before_relaxation) 
			{
				rejects = 0;
				// Relax the distance threshold
				distance_threshold *= relaxation_coefficient;
			}
			continue; 
		}

		// Accept the sample
		accepted_samples.push_back(sample);
	}

	return move(accepted_samples);
}


renderer::renderer( 
	shared_ptr<black_label::renderer::camera> camera_,
	path shader_directory, 
	path asset_directory )
	: shader_directory(shader_directory)
	, asset_directory(asset_directory)
	, camera{camera_}
MSVC_PUSH_WARNINGS(4355)
	, light_grid(64, *camera_, lights)
MSVC_POP_WARNINGS()
	, gpu_lights{usage::stream_draw, format::r32f}
	, next_shadow_map_id{0}
	, poisson_disc{generate_poisson_disc(128)}
	, light_buffer{target::uniform_buffer, usage::dynamic_draw, 128}
{
	if (GLEW_ARB_texture_storage)
		glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_FRAMEBUFFER_SRGB);
	glClearColor(0.9f, 0.934f, 1.0f, 1.0f);



////////////////////////////////////////////////////////////////////////////////
/// OpenColorIO Test
////////////////////////////////////////////////////////////////////////////////
	/*
	{
		using namespace ocio;
		using namespace gpu;
		
		auto transform = FileTransform::Create();
		transform->setSrc("LUTs/Kodak 5229 Vision2 Expression 500T.cube");
		transform->setInterpolation(INTERP_BEST);
		auto processor = Config::Create()->getProcessor(transform);

		GpuShaderDesc shader_description;
		const int lut_size = 32;
		shader_description.setLut3DEdgeLen(lut_size);
		container::darray<float> lut_data(3 * lut_size * lut_size * lut_size);
		processor->getGpuLut3D(lut_data.data(), shader_description);

		lut_texture = texture_3d(
			LINEAR, 
			CLAMP_TO_EDGE, 
			lut_size, 
			lut_size, 
			lut_size, 
			lut_data.data());
	}

	*/


	pipeline.import(shader_directory / "rendering_pipeline.json", shader_directory, camera);



	construct_screen_aligned_quad();

	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);


	int viewport_data[4];
	glGetIntegerv(GL_VIEWPORT, viewport_data);
	on_window_resized(viewport_data[2], viewport_data[3]);

}

renderer::~renderer()
{
	glDeleteFramebuffers(1, &framebuffer);
}



void renderer::update_lights()
{
	lights.clear();
	auto shadow_map_passes_guard = pipeline.shadow_map_passes;
	pipeline.shadow_map_passes.clear();
	next_shadow_map_id = 0;
	
	for (auto entry : locked_statics)
	{
		const auto& entities = entry.first;
		const auto& associated_models = entry.second;

		for (auto entity : *entities)
		{
			const auto& model = *associated_models[entity.index];
			const auto& model_matrix = entity.transformation();

			for (auto light : model.lights)
			{
				light.position = glm::vec3(model_matrix * glm::vec4(light.position, 1.0f));
				
				if (light_type::spot == light.type)
				{
					light.shadow_map = "shadow_map_" + to_string(next_shadow_map_id++);
					light.camera = make_shared<black_label::renderer::camera>(
						light.position, 
						light.position + light.direction, 
						glm::vec3{0.0, 1.0, 0.0},
						4096,
						4096,
						10.0f,
						10000.0f);

					pipeline.add_shadow_map_pass(light, shader_directory);

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



void renderer::import_model( model_identifier model )
{
	class import_model_task : public task
	{
	public:
		renderer& renderer_;
		path path_;
		gpu::model& gpu_model;


		import_model_task( renderer& renderer_, path path_, gpu::model& gpu_model )
			: renderer_{renderer_}, path_{path_}, gpu_model{gpu_model} {}

		task* execute()
		{
			auto original_path = path_;
			if (!canonical_and_preferred(path_, renderer_.asset_directory))
			{
				BOOST_LOG_TRIVIAL(warning) << "Missing model " << path_;
				return nullptr;
			}


			auto cpu_model = make_shared<cpu::model>(path_, defer_import);

			// No need to load unmodified models
			if (gpu_model.checksum == cpu_model->checksum)
				return nullptr;

			// Import the model
			if (cpu_model->import(path_))
			{
				renderer_.models_to_upload.push({original_path, cpu_model});
				for (const auto& mesh : cpu_model->meshes)
				{
					if (!mesh.material.diffuse_texture.empty())
						renderer_.import_texture(mesh.material.diffuse_texture);
					if (!mesh.material.specular_texture.empty())
						renderer_.import_texture(mesh.material.specular_texture);
				}
			}

			return nullptr;
		}
	};

	auto& gpu_model = models[model].lock();
	if (!gpu_model) 
	{
		models.erase(model);
		return;
	}

	auto& import_task = *new(task::allocate_root()) import_model_task{*this, model, *gpu_model};
	tbb::task::enqueue(import_task);
}



void renderer::import_texture( path path_ )
{
	class import_texture_task : public task
	{
	public:
		renderer& renderer_;
		const path path_;
		texture& gpu_texture;


		import_texture_task( renderer& renderer_, const path& path_, texture& gpu_texture )
			: renderer_{renderer_}, path_{path_}, gpu_texture{gpu_texture} {}

		task* execute()
		{
			auto cpu_texture = make_shared<cpu::texture>(path_, defer_import);

			// No need to load unmodified textures
			if (gpu_texture.checksum == cpu_texture->checksum)
				return nullptr;

			// Import the texture
			if (cpu_texture->import(path_))
				renderer_.textures_to_upload.push({path_, cpu_texture});

			return nullptr;
		}
	};

	texture_container::accessor accessor;
	textures.insert(accessor, {path_, make_shared<texture>()});
	auto& gpu_texture = *accessor->second;
	
	auto& import_task = *new(task::allocate_root()) import_texture_task{*this, path_, gpu_texture};
	tbb::task::enqueue(import_task);
}

bool renderer::reload_model( path path )
{
	if (models.end() == models.find(path)) return false;

	BOOST_LOG_TRIVIAL(info) << "Reloading model: " << path;
	import_model(std::move(path));
	return true;
}

bool renderer::reload_texture( path path )
{
	canonical_and_preferred(path, asset_directory);

	if (!textures.count(path)) return false;

	BOOST_LOG_TRIVIAL(info) << "Reloading texture: " << path;
	import_texture(std::move(path));
	return true;
}

bool renderer::reload_shader( path path )
{
	canonical_and_preferred(path, shader_directory);

	// Only handle shader files
	if (".glsl" != path.extension()) return false;

	auto reload_shader_helper = [this, &path] ( auto& program ) {
		if (path == program.vertex.path_to_shader)
		{
			BOOST_LOG_TRIVIAL(info) << "Reloading shader: " << path;
			//BOOST_LOG_SCOPED_LOGGER_TAG(this->log, "MultiLine", bool, true);

			if (program.is_complete())
				glDetachShader(program.id, program.vertex.id);

			program.vertex.load();
		
			if (program.is_complete())
			{
				glAttachShader(program.id, program.vertex.id);
				program.link();
			}
		}
		else if (path == program.fragment.path_to_shader)
		{
			BOOST_LOG_TRIVIAL(info) << "Reloading shader: " << path;
			//BOOST_LOG_SCOPED_LOGGER_TAG(this->log, "MultiLine", bool, true);

			if (program.is_complete())
				glDetachShader(program.id, program.fragment.id);

			program.fragment.load();

			if (program.is_complete())
			{
				glAttachShader(program.id, program.fragment.id);
				program.link();
			}
		}

		auto info_log = program.get_aggregated_info_log();
		if (!info_log.empty())
			BOOST_LOG_TRIVIAL(error) << info_log;
	};

	// Look up path to find the associated programs
	auto program_range = pipeline.programs.equal_range(path);
	if (program_range.first == program_range.second) return false;

	// Reload each associated program
	for (auto program = program_range.first; program_range.second != program; ++program) 
		reload_shader_helper(*program->second.lock());

	return true;
}

void renderer::construct_screen_aligned_quad()
{
	using namespace gpu::argument;

	screen_aligned_quad = mesh{
		vertices{
			1.0f, 1.0f, 0.0f,
			-1.0f, 1.0f, 0.0f,
			-1.0f, -1.0f, 0.0f,
			1.0f, -1.0f, 0.0f}
		+ indices{
			0u, 1u, 2u, 2u, 3u, 0u}};

	camera_ray_buffer = buffer{target::array, usage::stream_draw, sizeof(float) * 3 * 4};
	update_camera_ray_buffer();
}

void renderer::update_camera_ray_buffer()
{
	auto tan_alpha = tan(camera->fovy * 0.5);
	auto x = tan_alpha * camera->z_far;
	auto y = x / camera->aspect_ratio;

	glm::vec3 directions[4] = {
		glm::vec3(x, y, -camera->z_far),
		glm::vec3(-x, y, -camera->z_far),
		glm::vec3(-x, -y, -camera->z_far),
		glm::vec3(x, -y, -camera->z_far)
	};

	auto inverse_view_matrix = glm::inverse(glm::mat3(camera->view_matrix));

	directions[0] = inverse_view_matrix * directions[0];
	directions[1] = inverse_view_matrix * directions[1];
	directions[2] = inverse_view_matrix * directions[2];
	directions[3] = inverse_view_matrix * directions[3];

	screen_aligned_quad.vertex_array.bind();
	unsigned int index = 1;
	screen_aligned_quad.vertex_array.add_attribute(index, 3, nullptr);
	camera_ray_buffer.bind_and_update(0, sizeof(float)* 3 * 4, directions);
}

void renderer::update_statics()
{
	// Process dirty (new or updated) entities
	for (weak_ptr<const world::entities> entities; dirty_statics.try_pop(entities);) {
		// Skip deleted entities
		auto locked_entities = entities.lock();
		if (!locked_entities) continue;

		// Search for the entities among the existing statics
		auto existing = find_if(statics.cbegin(), statics.cend(),
			[locked_entities] ( auto entry )
			{ return entry.first.lock() == locked_entities; });

		// Associated models
		vector<shared_ptr<gpu::model>> associated_models;
		associated_models.reserve(locked_entities->size);
		for (auto entity : *locked_entities)
		{
			auto& weak_ptr_to_model = models[entity.model()];
			auto shared_ptr_to_model = weak_ptr_to_model.lock();
			if (!shared_ptr_to_model)
				weak_ptr_to_model = shared_ptr_to_model = make_shared<gpu::model>();

			associated_models.emplace_back(std::move(shared_ptr_to_model));
			// Mark the entity's model as dirty
			dirty_models.push(entity.model());
		}

		// Existing entities
		if (statics.cend() != existing)
			existing->second = associated_models;
		// New entities
		else
			statics.emplace_back(entities, associated_models);
	}
}

void renderer::optimize_for_rendering( world::group& group )
{ for (auto& entities : group) entities->sort_by_model(); }

renderer::locked_statics_guard renderer::lock_statics()
{
	for (auto entry = statics.begin(); statics.end() != entry; ++entry) {
		auto& entities = entry->first;

		// Acquire lock
		auto locked_entities = entities.lock();

		// Remove deleted entities
		if (!locked_entities) {
			statics.erase(entry);
			continue;
		};

		// Store successfully locked statics
		locked_statics.emplace_back(locked_entities, entry->second);
	}

	return locked_statics_guard{&locked_statics};
}



void renderer::render_pass( const pass& pass ) const
{
	unsigned int texture_unit{0}, binding_point{0};

	// Camera
	auto& camera = *pass.camera;
	glViewport(0, 0, camera.window.x, camera.window.y);

	// Depth test
	if (pass.test_depth) glEnable(GL_DEPTH_TEST);
	else glDisable(GL_DEPTH_TEST);

	// Framebuffer
	vector<GLenum> draw_buffers;
	glBindFramebuffer(GL_FRAMEBUFFER, (pass.output.empty()) ? 0 : framebuffer);
	
	if (!pass.output.empty())
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, 0, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, 0, 0);
		// TODO: Unbind all attachments.

	}

	int index = 0;
	for (const auto& entry : pass.output)
	{
		const auto& buffer = *entry.second;
		
		GLenum attachment = (format::is_depth_format(buffer.format))
			? GL_DEPTH_ATTACHMENT : GL_COLOR_ATTACHMENT0 + index++;

		glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, buffer, 0);
		if (!format::is_depth_format(buffer.format)) draw_buffers.push_back(attachment);
	}
	if (!draw_buffers.empty())
		glDrawBuffers(static_cast<int>(draw_buffers.size()), draw_buffers.data());

	GLenum framebuffer_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (GL_FRAMEBUFFER_COMPLETE != framebuffer_status)
	{
		BOOST_LOG_TRIVIAL(error) << "The framebuffer is not complete.";
		return;
	}

	// Clear
	glClear(pass.clearing_mask);
	
	// Use program
	pass.program_->use();

	// Input buffers
	for (const auto& entry : pass.input)
	{
		const auto& buffer_name = entry.first;
		const auto& buffer = *entry.second;
		buffer.use(*pass.program_, buffer_name.c_str(), texture_unit);
	}

	// Uniforms
	pass.program_->set_uniform("z_near", camera.z_near);
	pass.program_->set_uniform("z_far", camera.z_far);
	pass.program_->set_uniform("view_matrix", camera.view_matrix);
	pass.program_->set_uniform("projection_matrix", camera.projection_matrix);
	pass.program_->set_uniform("view_projection_matrix", camera.view_projection_matrix);
	pass.program_->set_uniform("inverse_view_projection_matrix", glm::inverse(camera.view_projection_matrix));
	pass.program_->set_uniform("wc_camera_eye_position", camera.eye);
	pass.program_->set_uniform("window_dimensions", camera.window.x, camera.window.y);
	pass.program_->set_uniform("poisson_disc", poisson_disc);

	int viewport_data[4];
	glGetIntegerv(GL_VIEWPORT, viewport_data);
	auto viewport_size = glm::floor(glm::vec2(viewport_data[2], viewport_data[3]));
	//glViewport(0, 0, static_cast<GLsizei>(viewport_size.x), static_cast<GLsizei>(viewport_size.y));
	pass.program_->set_uniform("tc_window", viewport_size);

	static vector<float> test_lights;
	test_lights.clear();
	for (auto light : lights)
	{
		test_lights.push_back(light.position.x);
		test_lights.push_back(light.position.y);
		test_lights.push_back(light.position.z);
		test_lights.push_back(light.color.r);
		test_lights.push_back(light.color.g);
		test_lights.push_back(light.color.b);

		if (light.shadow_map.empty()) continue;

		const auto& shadow_map = pipeline.buffers.at(light.shadow_map);
		const auto& light_camera = *light.camera;

		shadow_map.lock()->use(*pass.program_, light.shadow_map.c_str(), texture_unit);

		auto identifier = light.shadow_map.substr(string{"shadow_map_"}.size());
		pass.program_->set_uniform_block("light_block", binding_point, light_buffer);
	}

	if (!lights.empty())
		gpu_lights.bind_buffer_and_update(test_lights.size(), test_lights.data());
	else
		gpu_lights.bind_buffer_and_update<float>(1);

	gpu_lights.use(*pass.program_, "lights", texture_unit);



#ifndef USE_TILED_SHADING
	pass.program_->set_uniform("lights_size", static_cast<int>(lights.size()));
#endif



	// Statics
	if (pass.render_statics)
		for (auto entry : locked_statics)
		{
			const auto& entities = entry.first;
			const auto& associated_models = entry.second;

			for (auto entity : *entities)
			{
				const auto& model = *associated_models[entity.index];
				const auto& model_matrix = entity.transformation();

				// Uniforms
				pass.program_->set_uniform("normal_matrix", 
					glm::inverseTranspose(glm::mat3(model_matrix)));
				pass.program_->set_uniform("model_view_matrix", 
					camera.view_matrix * model_matrix);
				pass.program_->set_uniform("model_view_projection_matrix", 
					camera.view_projection_matrix * model_matrix);

				model.render(*pass.program_, texture_unit);
			}
		}

	// Screen-aligned quad
	if (pass.render_screen_aligned_quad)
		screen_aligned_quad.render_without_material();
}






void renderer::render_frame()
{
	assert(0 == glGetError());

	// Don't render if a program is incomplete (didn't compile or link)
	for (auto& program : pipeline.programs)
		if (!program.second.lock()->is_complete())
			return;

	

////////////////////////////////////////////////////////////////////////////////
/// Scene Update
////////////////////////////////////////////////////////////////////////////////
	update_camera_ray_buffer();
	update_statics();
	auto locked_statics_guard = lock_statics();



////////////////////////////////////////////////////////////////////////////////
/// Model Loading
////////////////////////////////////////////////////////////////////////////////
	for (model_identifier model; dirty_models.try_pop(model);)
		import_model(model);

	bool lights_need_update{false};
	for (models_to_upload_container::value_type entry; models_to_upload.try_pop(entry);)
	{
		auto model = models.at(entry.first).lock();
		if (!model) continue;

		*model = gpu::model{*entry.second, textures};
		if (model->has_lights()) lights_need_update = true;
	}
	if (lights_need_update) update_lights();

	for (textures_to_upload_container::value_type entry; textures_to_upload.try_pop(entry);)
	{
		texture_container::accessor accessor;
		if (!textures.find(accessor, entry.first))
			assert(false);
		
		auto& gpu_texture = *accessor->second;
		auto& cpu_texture = *entry.second;

		gpu_texture = texture{cpu_texture};
	}



////////////////////////////////////////////////////////////////////////////////
/// Render
////////////////////////////////////////////////////////////////////////////////
	for (const auto& pass : pipeline.shadow_map_passes) render_pass(pass);
	for (const auto& pass : pipeline.passes) render_pass(pass);



	// Block until every OpenGL call has been processed.
	glFinish();
}


void renderer::on_window_resized( int width, int height )
{
	camera->on_window_resized(width, height);
	light_grid.on_window_resized(width, height);

	//path path{"lighting.fragment.glsl"};
	//canonical_and_preferred(path, shader_directory);
	//
	//auto& lighting = *programs.find(path)->second;
//	if (lighting.is_complete())
//	{
//		lighting.use();
//
//#ifdef USE_TILED_SHADING
//		lighting.set_uniform("tile_size", light_grid.tile_size());
//		lighting.set_uniform("grid_dimensions", light_grid.tiles_x(), light_grid.tiles_y());
//#endif
//		lighting.set_uniform("window_dimensions", camera.window.x, camera.window.y);
//	}
	

	for (const auto& buffer : pipeline.buffers)
		if ("random" != buffer.first)
			buffer.second.lock()->bind_and_update(width, height);
	//shadow_map.bind_and_update(width, height);
}

} // namespace renderer
} // namespace black_label
