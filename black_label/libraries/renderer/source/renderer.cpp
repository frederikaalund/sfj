#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/renderer.hpp>

#include <black_label/file_buffer.hpp>
#include <black_label/renderer/storage/cpu/model.hpp>
#include <black_label/renderer/storage/file_loading.hpp>
#include <black_label/utility/checksum.hpp>

#include <cstdlib>
#include <cstdint>
#include <utility>
#include <fstream>
#include <iostream>

#include <boost/log/common.hpp>
#include <boost/math/constants/constants.hpp>

#include <GL/glew.h>
#ifndef APPLE
#include <GL/gl.h>
#else
#include <OpenGL/gl.h>
#endif

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>



#define USE_TILED_SHADING



using namespace std;



namespace black_label
{
namespace renderer
{

using namespace storage;
using namespace utility;



renderer::glew_setup::glew_setup()
{
	glewExperimental = GL_TRUE;
	GLenum glew_error = glewInit();
	glGetError(); // Unfortunately, GLEW produces an error for 3.2 contexts. Calling glGetError() dismisses it.
  
	if (GLEW_OK != glew_error)
		throw runtime_error("Glew failed to initialize.");

	if (!GLEW_VERSION_3_2)
		throw runtime_error("Requires OpenGL version 3.2 or above.");

	GLint max_draw_buffers;
	glGetIntegerv(GL_MAX_DRAW_BUFFERS, &max_draw_buffers);
	if (3 > max_draw_buffers)
		throw runtime_error("Requires at least 2 draw buffers.");
}



renderer::renderer( 
	const world_type& world, 
	black_label::renderer::camera&& camera ) 
	: camera(camera)
	, world(world)
	, models(new gpu::model[world.static_entities.models.capacity()])
	, sorted_statics(world.static_entities.capacity)
	, sorted_dynamics(world.dynamic_entities.capacity)
	, lights(1024 * 10)
MSVC_PUSH_WARNINGS(4355)
	, light_grid(64, this->camera, lights)
MSVC_POP_WARNINGS()
{
	glGenBuffers(1, &lights_buffer);
	glGenTextures(1, &lights_texture);

  

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);



	{
#ifdef USE_TILED_SHADING
#define UBERSHADER_TILED_SHADING_DEFINE "#define USE_TILED_SHADING\n"
#else
#define UBERSHADER_TILED_SHADING_DEFINE
#endif

		BOOST_LOG_SCOPED_LOGGER_TAG(log, "MultiLine", bool, true);

		buffering = program("buffering.vertex.glsl", nullptr, "buffering.fragment.glsl", "#version 150\n");
		BOOST_LOG_SEV(log, info) << buffering.get_aggregated_info_log();
		
		ubershader = program("test.vertex.glsl", nullptr, "test.fragment.glsl", "#version 150\n" UBERSHADER_TILED_SHADING_DEFINE);
		BOOST_LOG_SEV(log, info) << ubershader.get_aggregated_info_log();
    /*
		blur_horizontal = program("tone_mapper.vertex", nullptr, "blur_horizontal.fragment");
		BOOST_LOG_SEV(log, info) << blur_horizontal.get_aggregated_info_log();
		blur_vertical = program("tone_mapper.vertex", nullptr, "blur_vertical.fragment");
		BOOST_LOG_SEV(log, info) << blur_vertical.get_aggregated_info_log();
		tone_mapper = program("tone_mapper.vertex", nullptr, "tone_mapper.fragment");
		BOOST_LOG_SEV(log, info) << tone_mapper.get_aggregated_info_log();
     */
	}



	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	glGenRenderbuffers(1, &depth_renderbuffer);


  
	glGenTextures(1, &main_render);
	glBindTexture(GL_TEXTURE_2D, main_render);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &ws_positions);
	glBindTexture(GL_TEXTURE_2D, ws_positions);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &ss_positions);
	glBindTexture(GL_TEXTURE_2D, ss_positions);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &ss_normals);
	glBindTexture(GL_TEXTURE_2D, ss_normals);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &depths);
	glBindTexture(GL_TEXTURE_2D, depths);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &bloom1);
	glBindTexture(GL_TEXTURE_2D, bloom1);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

	glGenTextures(1, &bloom2);
	glBindTexture(GL_TEXTURE_2D, bloom2);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);



	int viewport_data[4];
	glGetIntegerv(GL_VIEWPORT, viewport_data);
	on_window_resized(viewport_data[2], viewport_data[3]);
}

renderer::~renderer()
{
	glDeleteBuffers(1, &lights_buffer);
	glDeleteTextures(1, &lights_texture);

	// TODO: Rest.
}



void renderer::update_lights()
{
	lights.clear();

	auto& entities = world.static_entities;
	for (auto entity = entities.cebegin(); entities.ceend() != entity; ++entity)
	{
		auto& model = models[entity.model_id()];
		auto& transformation = entity.transformation();
		if (model.has_lights())
			for_each(model.lights.cbegin(), model.lights.cend(), [&] ( const light& light ) {
				this->lights.push_back(black_label::renderer::light(
					glm::vec3(transformation * glm::vec4(light.position, 1.0f)),
					light.color,
					light.constant_attenuation,
					light.linear_attenuation,
					light.quadratic_attenuation));
			});
	}
}



bool renderer::import_model( const boost::filesystem::path& path, storage::gpu::model& gpu_model )
{
////////////////////////////////////////////////////////////////////////////////
/// There is no need to re-load unchanged models.
////////////////////////////////////////////////////////////////////////////////
	boost::crc_32_type::value_type checksum;
	{
		file_buffer::file_buffer model_file(path.string());
		if (model_file.empty()) return false;

		checksum = crc_32(model_file.data(), model_file.size());
		if (gpu_model.is_loaded() && gpu_model.model_file_checksum() == checksum) 
			return false;
	}



////////////////////////////////////////////////////////////////////////////////
/// The BLM file format is not standard but it imports quickly.
////////////////////////////////////////////////////////////////////////////////
	const auto blm_path = path.string() + ".blm";
	{
		ifstream blm_file(blm_path, ifstream::binary);
		if (blm_file.is_open() 
			&& gpu::model::peek_model_file_checksum(blm_file) == checksum)
		{
			blm_file >> gpu_model;
			BOOST_LOG_SEV(log, info) << "Imported model \"" << blm_path << "\"";

			if (gpu_model.has_lights()) update_lights();
			return true;
		}
	}



////////////////////////////////////////////////////////////////////////////////
/// The FBX file format is proprietary but has good application support.
/// In any case, Assimp will load most other files for us.
////////////////////////////////////////////////////////////////////////////////
	storage::cpu::model cpu_model(checksum);
	gpu_model.clear();

	if (!((".fbx" == path.extension() && load_fbx(path, cpu_model, gpu_model)) 
		|| load_assimp(path, cpu_model, gpu_model)))
		return false;
	
	if (gpu_model.has_lights()) update_lights();
	BOOST_LOG_SEV(log, info) << "Imported model " << path;



////////////////////////////////////////////////////////////////////////////////
/// Assimp processes models slowly so we export a BLM model file to save Assimp
/// its efforts in the future.
////////////////////////////////////////////////////////////////////////////////
	ofstream blm_file(blm_path, std::ofstream::binary);
	if (blm_file.is_open())
	{
		blm_file << cpu_model;
		BOOST_LOG_SEV(log, info) << "Exported model \"" << blm_path << "\"";
	}



	return true;
}

void renderer::import_model( model_id_type model_id )
{
	static const boost::filesystem::path MISSING_MODEL_PATH = "missing_model.dae";
	auto& path = world.models[model_id];
	auto& gpu_model = models[model_id];

	if (import_model(path, gpu_model)) return;

	BOOST_LOG_SEV(log, warning) << "Missing model \"" << path << "\"";
	import_model(MISSING_MODEL_PATH, gpu_model);
}



void renderer::render_frame()
{

////////////////////////////////////////////////////////////////////////////////
/// Model Loading
////////////////////////////////////////////////////////////////////////////////
	for (model_id_type model; dirty_models.dequeue(model);) import_model(model);

	

////////////////////////////////////////////////////////////////////////////////
/// Scene Update
////////////////////////////////////////////////////////////////////////////////
	for (entity_id_type entity; dirty_static_entities.dequeue(entity);)
	{
		if (find(sorted_statics.cbegin(), sorted_statics.cend(), entity) == sorted_statics.cend())
			sorted_statics.push_back(entity);

		if (models[world.static_entities.model_ids[entity]].has_lights())
			update_lights();
	}
	sort(sorted_statics.begin(), sorted_statics.end());


	for (entity_id_type entity; dirty_dynamic_entities.dequeue(entity);)
		if (find(sorted_dynamics.cbegin(), sorted_dynamics.cend(), entity) == sorted_dynamics.cend())
			sorted_dynamics.push_back(entity);
	sort(sorted_dynamics.begin(), sorted_dynamics.end());



////////////////////////////////////////////////////////////////////////////////
/// Rendering
////////////////////////////////////////////////////////////////////////////////	
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, ws_positions, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
		GL_TEXTURE_2D, ss_positions, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2,
		GL_TEXTURE_2D, ss_normals, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3,
		GL_TEXTURE_2D, depths, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	buffering.use();

	// Statics
	std::for_each(sorted_statics.cbegin(), sorted_statics.cend(), 
		[&] ( const entity_id_type id )
	{
		auto& model = models[this->world.static_entities.model_ids[id]];
		auto& model_matrix = this->world.static_entities.transformations[id];

		glUniformMatrix3fv(
			glGetUniformLocation(buffering.id, "normal_matrix"),
			1, GL_FALSE, glm::value_ptr(glm::inverseTranspose(glm::mat3(model_matrix))));

		glUniformMatrix4fv(
			glGetUniformLocation(buffering.id, "model_view_projection_matrix"),
			1, GL_FALSE, glm::value_ptr(this->camera.view_projection_matrix * model_matrix));

		glUniformMatrix4fv(
			glGetUniformLocation(buffering.id, "model_matrix"),
			1, GL_FALSE, glm::value_ptr(model_matrix));
    
		model.render(buffering.id);
	});

	// Dynamics
	std::for_each(sorted_dynamics.cbegin(), sorted_dynamics.cend(), 
		[&] ( const entity_id_type id )
	{
		auto& model = models[this->world.dynamic_entities.model_ids[id]];
		auto& model_matrix = this->world.dynamic_entities.transformations[id];

		glUniformMatrix3fv(
			glGetUniformLocation(buffering.id, "normal_matrix"),
			1, GL_FALSE, glm::value_ptr(glm::inverseTranspose(glm::mat3(model_matrix))));

		glUniformMatrix4fv(
			glGetUniformLocation(buffering.id, "model_view_projection_matrix"),
			1, GL_FALSE, glm::value_ptr(this->camera.view_projection_matrix * model_matrix));

		glUniformMatrix4fv(
			glGetUniformLocation(buffering.id, "model_matrix"),
			1, GL_FALSE, glm::value_ptr(model_matrix));

		model.render(buffering.id);
	});

	

////////////////////////////////////////////////////////////////////////////////
/// Frame setup
////////////////////////////////////////////////////////////////////////////////
	ubershader.use();
	
	auto lights_size = lights.size();

	for_each(sorted_dynamics.cbegin(), sorted_dynamics.cend(), 
		[&] ( const entity_id_type id )
	{
		auto& model = models[this->world.dynamic_entities.model_ids[id]];
		auto& model_matrix = this->world.dynamic_entities.transformations[id];

		auto& lights = this->lights;

		for_each(model.lights.cbegin(), model.lights.cend(), [&] ( const light light ) {
			lights.push_back(black_label::renderer::light(
				glm::vec3(model_matrix * glm::vec4(light.position, 1.0f)), 
				light.color,
				light.constant_attenuation,
				light.linear_attenuation,
				light.quadratic_attenuation));
		});
	});



	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, ws_positions);
	glUniform1i(glGetUniformLocation(ubershader.id, "ws_positions"), 0);
	
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, ss_positions);
	glUniform1i(glGetUniformLocation(ubershader.id, "ss_positions"), 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, ss_normals);
	glUniform1i(glGetUniformLocation(ubershader.id, "ss_normals"), 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, depths);
	glUniform1i(glGetUniformLocation(ubershader.id, "depths"), 3);
	
	auto err1 = glGetError();
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_BUFFER, lights_texture);
	auto err2 = glGetError();
	glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, lights_buffer);
	auto err3 = glGetError();

	glUniform1i(glGetUniformLocation(ubershader.id, "lights"), 4);
	glUniform1i(glGetUniformLocation(ubershader.id, "lights_size"), lights.size());

	if (!lights.empty())
	{
		glBindBuffer(GL_TEXTURE_BUFFER, lights_buffer);
		glBufferData(GL_TEXTURE_BUFFER, lights.size() * sizeof(light), lights.data(), GL_STREAM_DRAW);
	}

	glUniform1f(glGetUniformLocation(ubershader.id, "z_far"), camera.z_far);

	auto inverse_view_projection_matrix = glm::inverse(camera.view_projection_matrix);
	glUniformMatrix4fv(
		glGetUniformLocation(ubershader.id, "inverse_view_projection_matrix"),
		1, GL_FALSE, glm::value_ptr(inverse_view_projection_matrix));

	glUniformMatrix4fv(
		glGetUniformLocation(ubershader.id, "projection_matrix"),
		1, GL_FALSE, glm::value_ptr(camera.projection_matrix));

	glUniformMatrix4fv(
		glGetUniformLocation(ubershader.id, "view_matrix"),
		1, GL_FALSE, glm::value_ptr(camera.view_matrix));

	glUniformMatrix4fv(
		glGetUniformLocation(ubershader.id, "view_projection_matrix"),
		1, GL_FALSE, glm::value_ptr(camera.view_projection_matrix));

	

	

////////////////////////////////////////////////////////////////////////////////
/// Tiled Shading
////////////////////////////////////////////////////////////////////////////////
#ifdef USE_TILED_SHADING
	light_grid.update();
	light_grid.bind(ubershader.id);
#endif
	
	lights.resize(lights_size);



////////////////////////////////////////////////////////////////////////////////
/// Random Texture
////////////////////////////////////////////////////////////////////////////////
	static auto done = false;
	static unsigned int random_texture;
	if (!done)
	{
		done = true;

		glGenTextures(1, &random_texture);
		glBindTexture(GL_TEXTURE_2D, random_texture);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		auto random_data = container::darray<glm::vec3>(camera.window.x * camera.window.y);
		std::generate(random_data.begin(), random_data.end(), []() -> glm::vec3 { 
			glm::vec3 v;
			do 
			{
				v.x = static_cast<float>(rand()) / (RAND_MAX * 0.5f) - 1.0f;
				v.y = static_cast<float>(rand()) / (RAND_MAX * 0.5f) - 1.0f;
				v.z = static_cast<float>(rand()) / (RAND_MAX * 0.5f) - 1.0f;
			} while (glm::length(v) > 1.0f);
			return v;
		});

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F_ARB, camera.window.x, camera.window.y, 0,
			GL_RGB, GL_FLOAT, random_data.data());
	}

	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, random_texture);
	glUniform1i(glGetUniformLocation(ubershader.id, "random_texture"), 7);

	

////////////////////////////////////////////////////////////////////////////////
/// 2nd Pass
////////////////////////////////////////////////////////////////////////////////	
	static std::array<float, 12> screen_aligned_quad_vertices = {1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f};
	static storage::gpu::mesh screen_aligned_quad(material(), GL_QUADS, screen_aligned_quad_vertices.data(), &screen_aligned_quad_vertices.back() + 1);
	/*
	static auto done = false;
	static GLuint frustum_vbo;
	

	if (!done)
	{
		done = true;
		glGenBuffers(1, &frustum_vbo);
	}

	auto alpha = camera.fovy * boost::math::constants::pi<float>() / 360.0f;
	auto tan_alpha = tan(alpha);
	auto x = tan_alpha * camera.z_far;
	auto y = x / camera.aspect_ratio;

	glm::vec3 directions[4] = {
		glm::vec3(x, y, camera.z_far),
		glm::vec3(-x, y, camera.z_far),
		glm::vec3(-x, -y, camera.z_far),
		glm::vec3(x, -y, camera.z_far),
	};

	directions[0] = glm::mat3(camera.view_matrix) * directions[0];
	directions[1] = glm::mat3(camera.view_matrix) * directions[1];
	directions[2] = glm::mat3(camera.view_matrix) * directions[2];
	directions[3] = glm::mat3(camera.view_matrix) * directions[3];

	glBindVertexArray(screen_aligned_quad.vao);
	
	glBindBuffer(GL_ARRAY_BUFFER, frustum_vbo);
	glBufferData(
		GL_ARRAY_BUFFER, 
		sizeof(float) * 3 * 4,
		directions,
		GL_STREAM_DRAW);

	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr); 
	glEnableVertexAttribArray(2);
	
	glUniform3fv(glGetUniformLocation(ubershader.id, "camera_eye_position"), 3, glm::value_ptr(camera.eye));
	*/
	screen_aligned_quad.render(ubershader.id);
	


	/*
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, main_render);
	glUniform1i(glGetUniformLocation(ubershader.id, "main_render"), 0);
	*/



	/*
////////////////////////////////////////////////////////////////////////////////
/// Bloom Blur
////////////////////////////////////////////////////////////////////////////////
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, bloom2, 0);
	glClear(GL_DEPTH_BUFFER_BIT);
	
	blur_horizontal.use();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bloom1);
	glUniform1i(glGetUniformLocation(blur_horizontal.id, "bloom"), 0);

	glBegin(GL_QUADS);
	glVertex3f(1.0f, 1.0f, 0.0f);
	glVertex3f(-1.0f, 1.0f, 0.0f);
	glVertex3f(-1.0f, -1.0f, 0.0f);
	glVertex3f(1.0f, -1.0f, 0.0f);
	glEnd();

	

	glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, bloom1, 0);
	glClear(GL_DEPTH_BUFFER_BIT);

	blur_vertical.use();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bloom2);
	glUniform1i(glGetUniformLocation(blur_vertical.id, "bloom"), 0);

	glBegin(GL_QUADS);
	glVertex3f(1.0f, 1.0f, 0.0f);
	glVertex3f(-1.0f, 1.0f, 0.0f);
	glVertex3f(-1.0f, -1.0f, 0.0f);
	glVertex3f(1.0f, -1.0f, 0.0f);
	glEnd();



////////////////////////////////////////////////////////////////////////////////
/// Tone Mapping
////////////////////////////////////////////////////////////////////////////////
	glBindFramebufferEXT(GL_FRAMEBUFFER, 0);
	glClear(GL_DEPTH_BUFFER_BIT);

	tone_mapper.use();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, main_render);
	glUniform1i(glGetUniformLocation(tone_mapper.id, "main_render"), 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bloom1);
	glUniform1i(glGetUniformLocation(tone_mapper.id, "bloom"), 1);

	glBegin(GL_QUADS);
	glVertex3f(1.0f, 1.0f, 0.0f);
	glVertex3f(-1.0f, 1.0f, 0.0f);
	glVertex3f(-1.0f, -1.0f, 0.0f);
	glVertex3f(1.0f, -1.0f, 0.0f);
	glEnd();
	*/
	glFinish();
}

thread_pool::task renderer::render_frame_()
{
	using namespace thread_pool;
	task frame_task;



////////////////////////////////////////////////////////////////////////////////
/// Model Loading
////////////////////////////////////////////////////////////////////////////////
	for (model_id_type model; dirty_models.dequeue(model);) 
		frame_task |= [&, model](){ import_model(model); };



////////////////////////////////////////////////////////////////////////////////
/// Scene Update
////////////////////////////////////////////////////////////////////////////////
	task static_sorting([&] () 
	{
		for (entity_id_type entity; dirty_static_entities.dequeue(entity);)
		{
			if (find(sorted_statics.cbegin(), sorted_statics.cend(), entity) == sorted_statics.cend())
				sorted_statics.push_back(entity);

			if (models[this->world.static_entities.model_ids[entity]].has_lights())
				update_lights();
		}
		sort(sorted_statics.begin(), sorted_statics.end());
	});

	task dynamic_sorting([&] () 
	{
		for (entity_id_type entity; dirty_dynamic_entities.dequeue(entity);)
			if (find(sorted_dynamics.cbegin(), sorted_dynamics.cend(), entity) == sorted_dynamics.cend())
				sorted_dynamics.push_back(entity);
		sort(sorted_dynamics.begin(), sorted_dynamics.end());
	});

	frame_task >>= static_sorting | dynamic_sorting;



////////////////////////////////////////////////////////////////////////////////
/// Frame setup
////////////////////////////////////////////////////////////////////////////////
	auto lights_size = lights.size();
	
	for_each(sorted_dynamics.cbegin(), sorted_dynamics.cend(), 
		[&] ( const entity_id_type id )
	{
		auto& model = models[this->world.dynamic_entities.model_ids[id]];
		auto& model_matrix = this->world.dynamic_entities.transformations[id];

		auto& lights = this->lights;

		for_each(model.lights.cbegin(), model.lights.cend(), [&] ( const light light ) {
			lights.push_back(black_label::renderer::light(
				glm::vec3(model_matrix * glm::vec4(light.position, 1.0f)), 
				light.color,
				light.constant_attenuation,
				light.linear_attenuation,
				light.quadratic_attenuation));
		});
	});
	
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, main_render, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_BUFFER, lights_texture);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, lights_buffer);
	glUniform1i(glGetUniformLocation(ubershader.id, "lights"), 1);
	glUniform1i(glGetUniformLocation(ubershader.id, "lights_size"), lights.size());

	if (!lights.empty())
	{
		glBindBuffer(GL_TEXTURE_BUFFER, lights_buffer);
		glBufferData(GL_TEXTURE_BUFFER, lights.size() * sizeof(light), lights.data(), GL_STREAM_DRAW);
	}



	return move(frame_task);
}

void renderer::on_window_resized( int width, int height )
{
	camera.on_window_resized(width, height);
	light_grid.on_window_resized(width, height);

	glViewport(0, 0, width, height);


	ubershader.use();
	glUniform1i(glGetUniformLocation(ubershader.id, "tile_size"), light_grid.tile_size());
	glUniform2i(glGetUniformLocation(ubershader.id, "window_dimensions"), camera.window.x, camera.window.y);
	glUniform2i(glGetUniformLocation(ubershader.id, "grid_dimensions"), light_grid.tiles_x(), light_grid.tiles_y());


	glBindRenderbuffer(GL_RENDERBUFFER, depth_renderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);

	glBindTexture(GL_TEXTURE_2D, main_render);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	glBindTexture(GL_TEXTURE_2D, ws_positions);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	glBindTexture(GL_TEXTURE_2D, ss_positions);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	glBindTexture(GL_TEXTURE_2D, ss_normals);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	glBindTexture(GL_TEXTURE_2D, depths);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	glBindTexture(GL_TEXTURE_2D, bloom1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	glBindTexture(GL_TEXTURE_2D, bloom2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, nullptr);


	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, main_render, 0);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
		GL_TEXTURE_2D, bloom1, 0);


	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
		GL_RENDERBUFFER, depth_renderbuffer);


	GLenum draw_buffer[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
	glDrawBuffersARB(4, draw_buffer);


	GLenum framebuffer_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (GL_FRAMEBUFFER_COMPLETE != framebuffer_status)
		throw runtime_error("The framebuffer is not complete.");
}

} // namespace renderer
} // namespace black_label



/*
auto world_to_window_coordinates = [&] ( glm::vec3 v ) -> glm::vec3
{
	auto v_clip = view_projection_matrix * glm::vec4(v, 1.0f);
	auto v_ndc = glm::vec3(v_clip / v_clip.w);
	return glm::vec3((v_ndc.x+1.0f)*width_f*0.5f, (v_ndc.y+1.0f)*height_f*0.5f, v_ndc.z);
};
	
 
 
 
 float
 alpha = camera.fovy * boost::math::constants::pi<float>() / 360.0f,
 tan_alpha = tan(alpha),
 sphere_factor_y = 1.0f / cos(alpha),
 beta = atan(tan_alpha * camera.aspect_ratio),
 sphere_factor_x = 1.0f / cos(beta);
 
auto sphere_in_frustrum = [&] ( const glm::vec3& p, float radius ) -> bool
{
	float d1,d2;
	float az,ax,ay,zz1,zz2;
		
	glm::vec3 v = p - this->camera.eye;

	az = glm::dot(v, this->camera.forward());
	if (az > z_far + radius || az < z_near - radius)
		return false;

	ax = glm::dot(v, this->camera.right());
	zz1 = az * tan_alpha * aspect_ratio;
	d1 = sphere_factor_x * radius;
	if (ax > zz1+d1 || ax < -zz1-d1)
		return false;

	ay = glm::dot(v, this->camera.up());
	zz2 = az * tan_alpha;
	d2 = sphere_factor_y * radius;
	if (ay > zz2+d2 || ay < -zz2-d2)
		return false;

	return true;
};
*/