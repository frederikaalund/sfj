#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/renderer.hpp>

#include <black_label/file_buffer.hpp>
#include <black_label/renderer/storage/cpu/model.hpp>

#include <cstdlib>
#include <utility>
#include <fstream>
#include <iostream>

#include <boost/crc.hpp>
#include <boost/log/common.hpp>

#include <GL/glew.h>
#ifndef APPLE
#include <GL/gl.h>
#else
#include <OpenGL/gl.h>
#endif

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>



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
	GLenum glew_error = glewInit();

	if (GLEW_OK != glew_error)
		throw runtime_error("Glew failed to initialize.");

	if (!GLEW_VERSION_3_2)
		throw runtime_error("Requires OpenGL version 3.2 or above.");

	GLint max_draw_buffers;
	glGetIntegerv(GL_MAX_DRAW_BUFFERS, &max_draw_buffers);
	if (2 > max_draw_buffers)
		throw runtime_error("Requires at least 2 draw buffers.");
}



renderer::renderer( 
	const world_type& world, 
	black_label::renderer::camera&& camera ) 
	: camera(camera)
  , world(world)
	, models(new gpu::model[world.static_entities.models.capacity()])
	, sorted_statics(world.static_entities.capacity)
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

		ubershader = program("test.vertex.glsl", nullptr, "test.fragment.glsl", "#version 150\n" UBERSHADER_TILED_SHADING_DEFINE);
		BOOST_LOG_SEV(log, info) << ubershader.get_aggregated_info_log();
		blur_horizontal = program("tone_mapper.vertex", nullptr, "blur_horizontal.fragment");
		BOOST_LOG_SEV(log, info) << blur_horizontal.get_aggregated_info_log();
		blur_vertical = program("tone_mapper.vertex", nullptr, "blur_vertical.fragment");
		BOOST_LOG_SEV(log, info) << blur_vertical.get_aggregated_info_log();
		tone_mapper = program("tone_mapper.vertex", nullptr, "tone_mapper.fragment");
		BOOST_LOG_SEV(log, info) << tone_mapper.get_aggregated_info_log();
	}



	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	glGenRenderbuffers(1, &depth_renderbuffer);

	glGenTextures(1, &main_render);
	glBindTexture(GL_TEXTURE_2D, main_render);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

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
					light.radius,
					light.color));
			});
	}

	lights.push_back(light(glm::vec3(0.0f, 5.0f, 0.0f), 10.0f, glm::vec4(1.0f, 0.0f, 1.0f, 1.0f)));

	if (!lights.empty())
	{
		glBindBuffer(GL_TEXTURE_BUFFER, lights_buffer);
		glBufferData(GL_TEXTURE_BUFFER, lights.size() * sizeof(light), lights.data(), GL_STREAM_DRAW);
	}
}



void renderer::import_model( model_id_type model_id )
{
	const string& model_path = world.static_entities.models[model_id];
	auto& model = models[model_id];



////////////////////////////////////////////////////////////////////////////////
/// A checksum of the model file is useful when checking for changes.
////////////////////////////////////////////////////////////////////////////////
	file_buffer::file_buffer model_file(model_path);
	boost::crc_32_type::value_type checksum;
	{
		boost::crc_32_type crc;
		crc.process_bytes(model_file.data(), model_file.size());
		checksum = crc.checksum();
	}
	if (model.is_loaded() && model.model_file_checksum() == checksum) return;



////////////////////////////////////////////////////////////////////////////////
/// The BLM file format is not standard but it imports quickly.
////////////////////////////////////////////////////////////////////////////////
	const string blm_path = model_path + ".blm";
	{
		ifstream blm_file(blm_path, ifstream::binary);
		if (blm_file.is_open() 
			&& gpu::model::peek_model_file_checksum(blm_file) == checksum)
		{
			blm_file >> model;
			BOOST_LOG_SEV(log, info) << "Imported model \"" << blm_path << "\"";
			
			if (model.has_lights()) update_lights();
			return;
		}
	}
	


////////////////////////////////////////////////////////////////////////////////
/// In any case, Assimp will load most other files for us.
////////////////////////////////////////////////////////////////////////////////
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFileFromMemory(
		model_file.data(), 
		model_file.size(),
		aiProcess_CalcTangentSpace       | 
		aiProcess_GenNormals			 |
		aiProcess_Triangulate            |
		aiProcess_JoinIdenticalVertices  |
		aiProcess_PreTransformVertices   |
		aiProcess_SortByPType);
	
	// If Assimp fails, we give up.
	if (!scene)	
	{
		BOOST_LOG_SEV(log, warning) << importer.GetErrorString();
		return;
	}

	storage::cpu::model cpu_model(checksum);
	model.clear();

	vector<unsigned int> indices;
	for (unsigned int m = 0; scene->mNumMeshes > m; ++m)
	{
		aiMesh* ai_mesh = scene->mMeshes[m];
		auto ai_vertices = ai_mesh->mVertices;
		auto ai_normals = ai_mesh->mNormals;
		auto ai_faces = ai_mesh->mFaces;

		const static int indices_per_face = 3;
		indices.clear();
		indices.reserve(ai_mesh->mNumFaces * indices_per_face);
		for (auto f = ai_faces; &ai_faces[ai_mesh->mNumFaces] != f; ++f)
		{
			// Assimp's triangulation should ensure the following.
			assert(indices_per_face == f->mNumIndices);

			indices.insert(indices.end(), 
				f->mIndices, 
				&f->mIndices[f->mNumIndices]);
		}

		aiMaterial* ai_material = scene->mMaterials[ai_mesh->mMaterialIndex];

		aiColor3D diffuse_color;
		ai_material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse_color);

		material material;
		material.diffuse.r = diffuse_color.r;
		material.diffuse.g = diffuse_color.g;
		material.diffuse.b = diffuse_color.b;

		ai_material->Get(AI_MATKEY_OPACITY, material.alpha);

		cpu::mesh cpu_mesh(
			std::move(material),
			GL_TRIANGLES,
			reinterpret_cast<float*>(ai_vertices), 
			reinterpret_cast<float*>(&ai_vertices[ai_mesh->mNumVertices]), 
			reinterpret_cast<float*>(&ai_normals[0]), 
			indices.data(),
			&indices.back()+1);
		
		model.push_back(gpu::mesh(cpu_mesh));
		cpu_model.push_back(std::move(cpu_mesh));
	}

	for (unsigned int l = 0; scene->mNumLights > l; ++l)
	{
		aiLight* ai_light = scene->mLights[l];

		auto position = glm::vec3(ai_light->mPosition[0], ai_light->mPosition[1], ai_light->mPosition[2]);
		auto color = glm::vec4(1.0f*std::rand()/RAND_MAX, 1.0f*std::rand()/RAND_MAX, 1.0f*std::rand()/RAND_MAX, 1.0f);

		auto light = black_label::renderer::light(position, 1.2f*std::rand()/RAND_MAX, color);

		model.lights.push_back(light);
		cpu_model.lights.push_back(light);
	}
	if (model.has_lights()) update_lights();

	BOOST_LOG_SEV(log, info) << "Imported model \"" << model_path << "\"";



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
}



void renderer::render_frame()
{

////////////////////////////////////////////////////////////////////////////////
/// Model Loading
////////////////////////////////////////////////////////////////////////////////
	for (model_id_type model; dirty_models.dequeue(model);) import_model(model);



////////////////////////////////////////////////////////////////////////////////
/// Scene Sorting
////////////////////////////////////////////////////////////////////////////////
	for (entity_id_type entity; dirty_static_entities.dequeue(entity);)
		if (find(sorted_statics.cbegin(), sorted_statics.cend(), entity)
			== sorted_statics.cend())
			sorted_statics.push_back(entity);
	sort(sorted_statics.begin(), sorted_statics.end());



////////////////////////////////////////////////////////////////////////////////
/// Frame setup
////////////////////////////////////////////////////////////////////////////////
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
	

	
////////////////////////////////////////////////////////////////////////////////
/// Tiled Shading
////////////////////////////////////////////////////////////////////////////////
#ifdef USE_TILED_SHADING
	light_grid.update();
	light_grid.bind(ubershader.id);
#endif



////////////////////////////////////////////////////////////////////////////////
/// Rendering
////////////////////////////////////////////////////////////////////////////////	
	ubershader.use();
	/*
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, main_render);
	glUniform1i(glGetUniformLocation(ubershader.id, "main_render"), 0);
	*/

	// Statics
	std::for_each(sorted_statics.cbegin(), sorted_statics.cend(), 
		[&] ( const entity_id_type id )
	{
		auto& model = models[this->world.static_entities.model_ids[id]];
		auto& model_matrix = this->world.static_entities.transformations[id];

		glUniformMatrix3fv(
			glGetUniformLocation(ubershader.id, "normal_matrix"),
			1, GL_FALSE, glm::value_ptr(glm::inverseTranspose(glm::mat3(model_matrix))));

		glUniformMatrix4fv(
			glGetUniformLocation(ubershader.id, "model_view_projection_matrix"),
			1, GL_FALSE, glm::value_ptr(this->camera.view_projection_matrix * model_matrix));

		glUniformMatrix4fv(
			glGetUniformLocation(ubershader.id, "model_matrix"),
			1, GL_FALSE, glm::value_ptr(model_matrix));

		model.render();
	});



	/*
	glBegin(GL_QUADS);
	glVertex3f(1.0f, 1.0f, 0.0f);
	glVertex3f(-1.0f, 1.0f, 0.0f);
	glVertex3f(-1.0f, -1.0f, 0.0f);
	glVertex3f(1.0f, -1.0f, 0.0f);
	glEnd();
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

void renderer::on_window_resized( int width, int height )
{
	camera.on_window_resized(width, height);
	light_grid.on_window_resized(width, height);

	glViewport(0, 0, width, height);


	glUseProgram(ubershader.id);
	glUniform1i(glGetUniformLocation(ubershader.id, "tile_size"), light_grid.tile_size());
	glUniform2i(glGetUniformLocation(ubershader.id, "window_dimensions"), camera.window.x, camera.window.y);
	glUniform2i(glGetUniformLocation(ubershader.id, "grid_dimensions"), light_grid.tiles_x(), light_grid.tiles_y());


	glBindRenderbuffer(GL_RENDERBUFFER, depth_renderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);

	glBindTexture(GL_TEXTURE_2D, main_render);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, 0);

	glBindTexture(GL_TEXTURE_2D, bloom1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, 0);

	glBindTexture(GL_TEXTURE_2D, bloom2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, 0);


	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, main_render, 0);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
		GL_TEXTURE_2D, bloom1, 0);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, 
		GL_RENDERBUFFER, depth_renderbuffer);


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