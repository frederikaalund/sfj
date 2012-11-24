#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/renderer.hpp>

#include <black_label/file_buffer.hpp>
#include <black_label/renderer/storage/cpu/model.hpp>
#include <black_label/renderer/storage/file_loading.hpp>
#include <black_label/utility/checksum.hpp>

#include <array>
#include <cstdlib>
#include <cstdint>
#include <utility>
#include <fstream>
#include <iostream>
#include <random>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
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



namespace black_label {
namespace renderer {

using namespace storage;
using namespace storage::gpu;
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
		throw runtime_error("Requires at least 3 draw buffers.");
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
	, main_render(NEAREST, CLAMP_TO_EDGE)
	, depths(NEAREST, CLAMP_TO_EDGE)
	, shadow_map(NEAREST, CLAMP_TO_EDGE)
	, wc_normals(NEAREST, CLAMP_TO_EDGE)
	, albedos(NEAREST, CLAMP_TO_EDGE)
	, bloom1(NEAREST, MIRRORED_REPEAT)
	, bloom2(NEAREST, MIRRORED_REPEAT)
	, gpu_lights(black_label::renderer::generate)
{
	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_FRAMEBUFFER_SRGB);

	glClearColor(0.9f, 0.934f, 1.0f, 1.0f);



	{
#ifdef USE_TILED_SHADING
#define LIGHTING_TILED_SHADING_DEFINE "#define USE_TILED_SHADING\n"
#else
#define LIGHTING_TILED_SHADING_DEFINE
#endif

	/*
	#pragma optionNV(fastmath on)
	#pragma optionNV(fastprecision on)
	//#pragma optionNV(ifcvt all)
	//#pragma optionNV(inline all)
	//#pragma optionNV(strict on)
	//#pragma optionNV(unroll all)
	 */

		BOOST_LOG_SCOPED_LOGGER_TAG(log, "MultiLine", bool, true);

		array<const char*, 2> buffering_output_names = {"normal", "albedo"};
		buffering = program("buffering.vertex.glsl", nullptr, "buffering.fragment.glsl", "#version 150\n", buffering_output_names.cbegin(), buffering_output_names.cend());
		auto buffer_info_log = buffering.get_aggregated_info_log();
		if (!buffer_info_log.empty())
			BOOST_LOG_SEV(log, info) << buffer_info_log;
		
		null = program("null.vertex.glsl", nullptr, nullptr, "#version 150\n");
		auto null_info_log = null.get_aggregated_info_log();
		if (!null_info_log.empty())
			BOOST_LOG_SEV(log, info) << null_info_log;

		array<const char*, 2> lighting_output_names = {"color", "overbright"};
		lighting = program("lighting.vertex.glsl", nullptr, "lighting.fragment.glsl", "#version 150\n" LIGHTING_TILED_SHADING_DEFINE, lighting_output_names.cbegin(), lighting_output_names.cend());
		auto lighting_info_log = lighting.get_aggregated_info_log();
		if (!lighting_info_log.empty())
			BOOST_LOG_SEV(log, info) << lighting_info_log;

		array<const char*, 1> blur_horizontal_output_names = {"result"};
		blur_horizontal = program("lighting.vertex.glsl", nullptr, "blur.fragment.glsl", "#version 150\n#define HORIZONTAL\n", blur_horizontal_output_names.cbegin(), blur_horizontal_output_names.cend());
		auto blur_horizontal_info_log = blur_horizontal.get_aggregated_info_log();
		if (!blur_horizontal_info_log.empty())
			BOOST_LOG_SEV(log, info) << blur_horizontal_info_log;

		array<const char*, 1> blur_vertical_output_names = {"result"};
		blur_vertical = program("lighting.vertex.glsl", nullptr, "blur.fragment.glsl", "#version 150\n#define VERTICAL\n", blur_vertical_output_names.cbegin(), blur_vertical_output_names.cend());
		auto blur_vertical_info_log = blur_vertical.get_aggregated_info_log();
		if (!blur_vertical_info_log.empty())
			BOOST_LOG_SEV(log, info) << blur_vertical_info_log;

		array<const char*, 1> tone_mapper_output_names = {"result"};
		tone_mapper = program("lighting.vertex.glsl", nullptr, "tone_mapper.fragment.glsl", "#version 150\n", tone_mapper_output_names.cbegin(), tone_mapper_output_names.cend());
		auto tone_mapper_info_log = tone_mapper.get_aggregated_info_log();
		if (!tone_mapper_info_log.empty())
			BOOST_LOG_SEV(log, info) << tone_mapper_info_log;
	}


	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);


	int viewport_data[4];
	glGetIntegerv(GL_VIEWPORT, viewport_data);
	on_window_resized(viewport_data[2], viewport_data[3]);


	mt19937 random_number_generator;
	uniform_real_distribution<float> distribution(-1.0f, 1.0f);
	int random_texture_size = 32;
	auto random_data = container::darray<glm::vec3>(random_texture_size * random_texture_size);
	std::generate(random_data.begin(), random_data.end(), [&] () -> glm::vec3 { 
		glm::vec3 v;
		do 
		{
			v.x = distribution(random_number_generator);
			v.y = distribution(random_number_generator);
			v.z = distribution(random_number_generator);
		} while (glm::length(v) > 1.0f);
		return v;
	});
	
	random_texture = texture_2d(NEAREST, REPEAT, 
		random_texture_size, random_texture_size, random_data.data(), 0);

	shadow_map.bind_and_update<texture::depth_float>(2048, 2048, nullptr, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
}

renderer::~renderer()
{
	glDeleteFramebuffers(1, &framebuffer);
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
		if (blm_file.is_open())
		{
			class test2
			{
			public:
				friend class boost::serialization::access;

				void serialize( boost::archive::binary_iarchive& archive, unsigned int version )
				{
					archive.load_binary(&cs, sizeof(cpu::checksum_type));
				}
				cpu::checksum_type cs;
			};
			test2 test2i;

			{
				boost::archive::binary_iarchive blm_archive(blm_file);
				blm_archive >> test2i;
			}

			if (test2i.cs == checksum)
			{
				blm_file.seekg(0);
				boost::archive::binary_iarchive blm_archive(blm_file);
				blm_archive >> gpu_model;
				BOOST_LOG_SEV(log, info) << "Imported model \"" << blm_path << "\"";

				if (gpu_model.has_lights()) update_lights();
				return true;
			}
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
		boost::archive::binary_oarchive blm_archive(blm_file);
		blm_archive << cpu_model;
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
	assert(0 == glGetError());



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
/// Shadow Map
////////////////////////////////////////////////////////////////////////////////
	int viewport_data[4];
	glGetIntegerv(GL_VIEWPORT, viewport_data);
	glViewport(0, 0, 2048, 2048);
	
	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
		GL_TEXTURE_2D, shadow_map, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 0, 0, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, 0, 0, 0);
	
	glDrawBuffersARB(0, nullptr);

	glClear(GL_DEPTH_BUFFER_BIT);

	null.use();


	auto light_source = lights[0].position;
	auto light_target = glm::vec3(0.0, 0.0, 0.0);
	auto light_projection_matrix = glm::perspective(camera.fovy, camera.aspect_ratio, 1000.0f, 5000.0f);
	auto light_view_matrix = glm::lookAt(light_source, light_target, camera.sky);;
	auto light_view_projection_matrix = light_projection_matrix * light_view_matrix;


	// Statics
	std::for_each(sorted_statics.cbegin(), sorted_statics.cend(), 
		[&] ( const entity_id_type id )
	{
		auto& model = models[this->world.static_entities.model_ids[id]];
		auto& model_matrix = this->world.static_entities.transformations[id];

		null.set_uniform("model_view_projection_matrix", 
			light_view_projection_matrix * model_matrix);

		model.render_without_material();
	});

	// Dynamics
	std::for_each(sorted_dynamics.cbegin(), sorted_dynamics.cend(), 
		[&] ( const entity_id_type id )
	{
		auto& model = models[this->world.dynamic_entities.model_ids[id]];
		auto& model_matrix = this->world.dynamic_entities.transformations[id];

		null.set_uniform("model_view_projection_matrix", 
			light_view_projection_matrix * model_matrix);

		model.render_without_material();
	});
	


////////////////////////////////////////////////////////////////////////////////
/// Rendering
////////////////////////////////////////////////////////////////////////////////	
	glViewport(0, 0, viewport_data[2], viewport_data[3]);
	
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
		GL_TEXTURE_2D, depths, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, wc_normals, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
		GL_TEXTURE_2D, albedos, 0);
	
	GLenum draw_buffer[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
	glDrawBuffersARB(2, draw_buffer);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	buffering.use();
	

	// Statics
	std::for_each(sorted_statics.cbegin(), sorted_statics.cend(), 
		[&] ( const entity_id_type id )
	{
		int texture_unit = 0;
		auto& model = models[this->world.static_entities.model_ids[id]];
		auto& model_matrix = this->world.static_entities.transformations[id];

		buffering.set_uniform("normal_matrix", 
			glm::inverseTranspose(glm::mat3(model_matrix)));
		buffering.set_uniform("model_view_projection_matrix", 
			this->camera.view_projection_matrix * model_matrix);

		model.render(buffering, texture_unit);
	});

	// Dynamics
	std::for_each(sorted_dynamics.cbegin(), sorted_dynamics.cend(), 
		[&] ( const entity_id_type id )
	{
		int texture_unit = 0;
		auto& model = models[this->world.dynamic_entities.model_ids[id]];
		auto& model_matrix = this->world.dynamic_entities.transformations[id];

		buffering.set_uniform("normal_matrix", 
			glm::inverseTranspose(glm::mat3(model_matrix)));
		buffering.set_uniform("model_view_projection_matrix", 
			this->camera.view_projection_matrix * model_matrix);

		model.render(buffering, texture_unit);
	});

	

////////////////////////////////////////////////////////////////////////////////
/// Frame setup
////////////////////////////////////////////////////////////////////////////////
	lighting.use();
	
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



	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, main_render, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
		GL_TEXTURE_2D, bloom1, 0);
	glDisable(GL_DEPTH_TEST);

	int texture_unit = 0;

	array<texture*, 5> texs = {&depths, &albedos, &wc_normals, &random_texture, &shadow_map};
	array<const char*, 5> names = {"depths", "albedos", "wc_normals", "random_texture", "shadow_map"};

	auto texs_it = texs.begin();
	auto names_it = names.cbegin();
	for (; texs.end() > texs_it; ++texs_it, ++names_it)
		(*texs_it)->use(lighting, *names_it, texture_unit);


	// TODO: Pack the light class tightly instead.
	static vector<float> test_lights;
	test_lights.clear();
	for_each(lights.cbegin(), lights.cend(), [&] ( const light& l ) {
		test_lights.push_back(l.position.x);
		test_lights.push_back(l.position.y);
		test_lights.push_back(l.position.z);
		test_lights.push_back(l.color.r);
		test_lights.push_back(l.color.g);
		test_lights.push_back(l.color.b);
		test_lights.push_back(l.constant_attenuation);
		test_lights.push_back(l.linear_attenuation);
		test_lights.push_back(l.quadratic_attenuation);
	});

	if (!lights.empty())
		gpu_lights.bind_buffer_and_update(test_lights.size(), test_lights.data());
	gpu_lights.use(lighting, "lights", texture_unit);
	 
#ifndef USE_TILED_SHADING
	lighting.set_uniform("lights_size", static_cast<int>(lights.size()));
#endif

	//lighting.set_uniform("z_near", camera.z_near);
	//lighting.set_uniform("z_far", camera.z_far);
	lighting.set_uniform("view_matrix", camera.view_matrix);
	//lighting.set_uniform("projection_matrix", camera.projection_matrix);	
	lighting.set_uniform("view_projection_matrix", 
		camera.view_projection_matrix);

	glm::mat4 light_tc_matrix(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.5f, 0.5f, 0.5f, 1.0f);
	lighting.set_uniform("light_matrix", light_tc_matrix * light_view_projection_matrix);



////////////////////////////////////////////////////////////////////////////////
/// Tiled Shading
////////////////////////////////////////////////////////////////////////////////
#ifdef USE_TILED_SHADING
	light_grid.update();
	light_grid.use(lighting, texture_unit);
#endif
	
	lights.resize(lights_size);

	

////////////////////////////////////////////////////////////////////////////////
/// 2nd Pass
////////////////////////////////////////////////////////////////////////////////	
	using namespace storage::gpu;
	
	static std::array<float, 12> screen_aligned_quad_vertices = {1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f};
	static mesh screen_aligned_quad(material(false), GL_QUADS, screen_aligned_quad_vertices.data(), &screen_aligned_quad_vertices.back() + 1);
	
	
	
	static auto done = false;
	static buffer<target::array, usage::stream_draw> frustrum(sizeof(float) * 3 * 4);
	

	
	auto alpha = camera.fovy * boost::math::constants::pi<float>() / 360.0f;
	auto tan_alpha = tan(alpha);
	auto x = tan_alpha * camera.z_far;
	auto y = x / camera.aspect_ratio;

	glm::vec3 directions[4] = {
		glm::vec3(x, y, -camera.z_far),
		glm::vec3(-x, y, -camera.z_far),
		glm::vec3(-x, -y, -camera.z_far),
		glm::vec3(x, -y, -camera.z_far),
	};

	auto inverse_view_matrix = glm::inverse(glm::mat3(camera.view_matrix));

	directions[0] = inverse_view_matrix * directions[0];
	directions[1] = inverse_view_matrix * directions[1];
	directions[2] = inverse_view_matrix * directions[2];
	directions[3] = inverse_view_matrix * directions[3];
	
	screen_aligned_quad.vertex_array.bind();

	frustrum.bind_and_update(0, sizeof(float) * 3 * 4, directions);

	if (!done)
	{
		done = true;
		unsigned int index = 3;
		screen_aligned_quad.vertex_array.add_attribute(index, 3, nullptr);
	}
	
	lighting.set_uniform("wc_camera_eye_position", camera.eye.x, camera.eye.y, camera.eye.z);
	
	screen_aligned_quad.render(lighting, texture_unit);



	
////////////////////////////////////////////////////////////////////////////////
/// Bloom Blur
////////////////////////////////////////////////////////////////////////////////
	
	// Horizontal
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloom2, 0);
	texture_unit = 0;
	blur_horizontal.use();
	bloom1.use(blur_horizontal, "source", texture_unit);
	screen_aligned_quad.render(lighting, texture_unit);
	
	// Vertical
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloom1, 0);
	texture_unit = 0;
	blur_vertical.use();
	bloom2.use(blur_vertical, "source", texture_unit);
	screen_aligned_quad.render(lighting, texture_unit);



////////////////////////////////////////////////////////////////////////////////
/// Tone Mapping
////////////////////////////////////////////////////////////////////////////////
	glBindFramebufferEXT(GL_FRAMEBUFFER, 0);
	tone_mapper.use();
	texture_unit = 0;

	main_render.use(tone_mapper, "main_render", texture_unit);
	bloom1.use(tone_mapper, "bloom", texture_unit);
	screen_aligned_quad.render(lighting, texture_unit);



	glFinish();
}

thread_pool::task renderer::render_frame_()
{
	using namespace thread_pool;
	task frame_task;

	return move(frame_task);
}

void renderer::on_window_resized( int width, int height )
{
	camera.on_window_resized(width, height);
	light_grid.on_window_resized(width, height);

	glViewport(0, 0, width, height);


	lighting.use();
#ifdef USE_TILED_SHADING
	//lighting.set_uniform("tile_size", light_grid.tile_size());
	//lighting.set_uniform("grid_dimensions", light_grid.tiles_x(), light_grid.tiles_y());
#endif
	//lighting.set_uniform("window_dimensions", camera.window.x, camera.window.y);


	main_render.bind_and_update<float>(width, height, nullptr, 0);
	depths.bind_and_update<texture::depth_float>(width, height, nullptr, 0);
	wc_normals.bind_and_update<float>(width, height, nullptr, 0);
	albedos.bind_and_update<float>(width, height, nullptr, 0);
	bloom1.bind_and_update<float>(width, height, nullptr, 0);
	bloom2.bind_and_update<float>(width, height, nullptr, 0);


	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, main_render, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
		GL_TEXTURE_2D, bloom1, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
		GL_TEXTURE_2D, depths, 0);


	GLenum draw_buffer[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
	glDrawBuffersARB(2, draw_buffer);

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