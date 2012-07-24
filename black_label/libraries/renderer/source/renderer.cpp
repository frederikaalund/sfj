#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/renderer.hpp>

#include <utility>
#include <iostream>

#include <GL/glew.h>
#include <GL/gl.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <assimp.hpp>
#include <aiScene.h>
#include <aiPostProcess.h>

using namespace std;



namespace black_label
{
namespace renderer
{

using namespace storage;



void swap( renderer& lhs, renderer& rhs )
{
	using std::swap;
	swap(lhs.world, rhs.world);
	swap(lhs.camera, rhs.camera);
	swap(lhs.static_models, rhs.static_models);
	swap(lhs.dynamic_models, rhs.dynamic_models);
	swap(lhs.sorted_statics, rhs.sorted_statics);
	swap(lhs.ubershader, rhs.ubershader);
	swap(lhs.blur_horizontal, rhs.blur_horizontal);
	swap(lhs.blur_vertical, rhs.blur_vertical);
	swap(lhs.tone_mapper, rhs.tone_mapper);
	swap(lhs.main_render, rhs.main_render);
	swap(lhs.bloom1, rhs.bloom1);
	swap(lhs.bloom2, rhs.bloom2);
	swap(lhs.framebuffer, rhs.framebuffer);
}

renderer::renderer( 
	const world::world* world, 
	black_label::renderer::camera&& camera ) 
	: world(world), camera(camera)
	, static_models(new model[world->static_entities.capacity])
	, dynamic_models(new model[world->dynamic_entities.capacity])
	, sorted_statics(world->static_entities.capacity)
{
	GLenum glew_error = glewInit();

	if (GLEW_OK != glew_error)
		throw runtime_error("Glew failed to initialize.");

	if (!GLEW_VERSION_2_0)
		throw runtime_error("Requires OpenGL version 2.0 or above.");

	if (!GLEW_EXT_framebuffer_object)
		throw runtime_error("Requires the framebuffer_object extension.");

	if (!GLEW_EXT_framebuffer_blit)
		throw runtime_error("Requires the framebuffer_blit extension.");
	
	if (!GLEW_ARB_texture_float)
		throw runtime_error("Requires the texture_float extension.");

	if (!GLEW_ARB_draw_buffers)
		throw runtime_error("Requires the draw_buffers extension.");
	
	GLint max_draw_buffers;
	glGetIntegerv(GL_MAX_DRAW_BUFFERS, &max_draw_buffers);
	if (2 > max_draw_buffers)
		throw runtime_error("Requires at least 2 draw buffers.");



	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);



	ubershader = storage::program("test.vertex", nullptr, "test.fragment");
	std::cout << ubershader.get_aggregated_info_log() << std::endl;
	blur_horizontal = storage::program("tone_mapper.vertex", nullptr, "blur_horizontal.fragment");
	std::cout << blur_horizontal.get_aggregated_info_log() << std::endl;
	blur_vertical = storage::program("tone_mapper.vertex", nullptr, "blur_vertical.fragment");
	std::cout << blur_vertical.get_aggregated_info_log() << std::endl;
	tone_mapper = storage::program("tone_mapper.vertex", nullptr, "tone_mapper.fragment");
	std::cout << tone_mapper.get_aggregated_info_log() << std::endl;



	glGenFramebuffersEXT(1, &framebuffer);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer);

	GLuint depth_renderbuffer;
	glGenRenderbuffersEXT(1, &depth_renderbuffer);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depth_renderbuffer);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, 800, 800);

	glGenTextures(1, &main_render);
	glBindTexture(GL_TEXTURE_2D, main_render);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, 800, 800, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, 0);

	glGenTextures(1, &bloom1);
	glBindTexture(GL_TEXTURE_2D, bloom1);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, 800, 800, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, 0);

	glGenTextures(1, &bloom2);
	glBindTexture(GL_TEXTURE_2D, bloom2);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, 800, 800, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, 0);



	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
		GL_TEXTURE_2D, main_render, 0);

	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT,
		GL_TEXTURE_2D, bloom1, 0);

	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, 
		GL_RENDERBUFFER_EXT, depth_renderbuffer);

	GLenum draw_buffer[] = {GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT};
	glDrawBuffersARB(2, draw_buffer);



	GLenum framebuffer_status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (GL_FRAMEBUFFER_COMPLETE != framebuffer_status)
		throw runtime_error("The framebuffer is not complete.");
}



bool renderer::load_static_model( id_type entity_id )
{
	bool is_initial_load = !static_models[entity_id].is_loaded();

	Assimp::Importer importer;
	const string& model_path = world->static_entities.models[entity_id];

	const aiScene* scene = importer.ReadFile(
		model_path, 
		aiProcess_CalcTangentSpace       | 
		aiProcess_GenNormals			 |
		aiProcess_Triangulate            |
		aiProcess_JoinIdenticalVertices  |
		aiProcess_PreTransformVertices   |
		aiProcess_SortByPType);
	
	// If the import failed, report it
	if (!scene)
	{
		string test = importer.GetErrorString();
		int asd = 2;
		//DoTheErrorLogging( importer.GetErrorString());
	}

	for (unsigned int m = 0; scene->mNumMeshes > m; ++m)
	{
		// Mesh
		aiMesh* ai_mesh = scene->mMeshes[m];

		// Vertices
		auto ai_vertices = ai_mesh->mVertices;

		// Normals
		auto ai_normals = ai_mesh->mNormals;

		// Indices
		auto ai_faces = ai_mesh->mFaces;

		const static int indices_per_face = 3;
		vector<unsigned int> indices;
		indices.reserve(ai_mesh->mNumFaces*indices_per_face);
		for (auto f = ai_faces; &ai_faces[ai_mesh->mNumFaces] != f; ++f)
		{
			assert(indices_per_face == f->mNumIndices);

			indices.insert(indices.end(), 
				f->mIndices, 
				&f->mIndices[f->mNumIndices]);
		}

		// Material
		aiMaterial* ai_material = scene->mMaterials[ai_mesh->mMaterialIndex];

		aiColor3D diffuse_color;
		ai_material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse_color);

		// Load model
		static_models[entity_id] = model(GL_TRIANGLES, 
			reinterpret_cast<float*>(ai_vertices), 
			reinterpret_cast<float*>(&ai_vertices[ai_mesh->mNumVertices]), 
			reinterpret_cast<float*>(&ai_normals[0]), 
			indices.begin()._Ptr,
			indices.end()._Ptr);

		color& diffuse = static_models[entity_id].material.diffuse;
		diffuse.r = diffuse_color.r;
		diffuse.g = diffuse_color.g;
		diffuse.b = diffuse_color.b;

		ai_material->Get(AI_MATKEY_OPACITY, 
			static_models[entity_id].material.alpha);
	}

	return is_initial_load;
}

template<typename loader_type>
int renderer::load_models( 
	dirty_id_container& ids_to_load, 
	container::svector<id_type>& loaded_ids, 
	const loader_type& loader )
{
	int loaded_models_count = 0;

	id_type id_to_load;
	while (ids_to_load.dequeue(id_to_load))
	{
		if (loader(id_to_load))
			loaded_ids.push_back(id_to_load);
		++loaded_models_count;
	}

	return loaded_models_count;
}

void renderer::render_frame()
{
////////////////////////////////////////////////////////////////////////////////
/// Model loading
////////////////////////////////////////////////////////////////////////////////
	
	// Statics
	if (load_models(dirty_static_entities, sorted_statics,
		[this] ( id_type id ) { return this->load_static_model(id); }))
	{ sort(sorted_statics.begin(), sorted_statics.end()); }



////////////////////////////////////////////////////////////////////////////////
/// Frame setup
////////////////////////////////////////////////////////////////////////////////
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
		GL_TEXTURE_2D, main_render, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


////////////////////////////////////////////////////////////////////////////////
/// Rendering
////////////////////////////////////////////////////////////////////////////////	
	auto view_projection_matrix = 
		glm::perspective(45.0f, 1.0f, 1.0f, 10000.0f) * camera.matrix;

	ubershader.use();
	


	// Statics
	for (auto id = sorted_statics.begin(); sorted_statics.end() != id; ++id)
	{
		auto& model = static_models[*id];
		auto& transformation = world->static_entities.transformations[*id];

		glColor4f(
			model.material.diffuse.r, 
			model.material.diffuse.g, 
			model.material.diffuse.b,
			model.material.alpha);

		glUniformMatrix3fv(
			glGetUniformLocation(ubershader.id, "normal_matrix"),
			1, GL_FALSE, glm::value_ptr(glm::inverseTranspose(glm::mat3() * glm::mat3(transformation))));

		glUniformMatrix4fv(
			glGetUniformLocation(ubershader.id, "model_view_projection_matrix"),
			1, GL_FALSE, glm::value_ptr(view_projection_matrix * transformation));

		model.render();
	}

	/*
////////////////////////////////////////////////////////////////////////////////
/// Bloom Blur
////////////////////////////////////////////////////////////////////////////////
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
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

	

	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
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
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
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

} // namespace renderer
} // namespace black_label
