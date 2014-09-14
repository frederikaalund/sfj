#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/renderer/pass.hpp>

#include <black_label/renderer/screen_aligned_quad.hpp>
#include <black_label/utility/algorithm.hpp>

#include <GL/glew.h>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/glm.hpp>



namespace black_label {
namespace renderer {

using namespace std;
using namespace gpu;



class poisson_disc
{
public:
	using data_type = vector<glm::vec2>;

	poisson_disc() : data(128)
	{ utility::generate_poisson_disc_samples(begin(data), end(data)); };

	operator data_type&() { return data; }

private:
	data_type data;
};



void pass::render( 
	const gpu::framebuffer& framebuffer, 
	const assets& assets ) const
{
	unsigned int texture_unit{0}, binding_point{0};

	// Camera
	glViewport(0, 0, camera->window.x, camera->window.y);

	// Depth test
	if (test_depth) glEnable(GL_DEPTH_TEST);
	else glDisable(GL_DEPTH_TEST);

	// Face culling
	switch(face_culling_mode) {
	case GL_FRONT:
	case GL_BACK:
	case GL_FRONT_AND_BACK:
		glEnable(GL_CULL_FACE);
		glCullFace(face_culling_mode);
		break;
	default:
		glDisable(GL_CULL_FACE);
		break;
	}

	// Framebuffer
	vector<GLenum> draw_buffers;
	glBindFramebuffer(GL_FRAMEBUFFER, (output_textures.empty()) ? 0 : framebuffer);
	
	if (!output_textures.empty())
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, 0, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, 0, 0);
		// TODO: Unbind all attachments.

	}

	int index = 0;
	for (const auto& entry : output_textures)
	{
		const auto& texture = *entry.second;
		
		GLenum attachment = (format::is_depth_format(texture.format))
			? GL_DEPTH_ATTACHMENT : GL_COLOR_ATTACHMENT0 + index++;

		glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture, 0);
		if (!format::is_depth_format(texture.format)) draw_buffers.push_back(attachment);
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
	glClear(clearing_mask);
	
	// Use program
	program_->use();

	// Input textures
	for (const auto& entry : input_textures)
	{
		const auto& texture_name = entry.first;
		const auto& texture = *entry.second;
		texture.use(*program_, texture_name.c_str(), texture_unit);
	}

	// Buffers
	for (const auto& entry : buffers)
	{
		const auto& buffer_name = entry.first;
		const auto& buffer = *entry.second;
		program_->set_shader_storage_block(buffer_name, binding_point, buffer);
	}

	// Uniforms
	program_->set_uniform("z_near", camera->z_near);
	program_->set_uniform("z_far", camera->z_far);
	program_->set_uniform("view_matrix", camera->view_matrix);
	program_->set_uniform("projection_matrix", camera->projection_matrix);
	program_->set_uniform("view_projection_matrix", camera->view_projection_matrix);
	program_->set_uniform("inverse_view_projection_matrix", glm::inverse(camera->view_projection_matrix));
	program_->set_uniform("wc_camera_eye_position", camera->eye);
	program_->set_uniform("window_dimensions", camera->window.x, camera->window.y);
	static poisson_disc poisson_disc;
	program_->set_uniform("poisson_disc", poisson_disc);

	int viewport_data[4];
	glGetIntegerv(GL_VIEWPORT, viewport_data);
	auto viewport_size = glm::floor(glm::vec2(viewport_data[2], viewport_data[3]));
	//glViewport(0, 0, static_cast<GLsizei>(viewport_size.x), static_cast<GLsizei>(viewport_size.y));
	program_->set_uniform("tc_window", viewport_size);

	static vector<float> test_lights;
	test_lights.clear();
	int shadow_map_index{0};
	for (auto light : assets.lights)
	{
		test_lights.push_back(light.position.x);
		test_lights.push_back(light.position.y);
		test_lights.push_back(light.position.z);
		test_lights.push_back(light.color.r);
		test_lights.push_back(light.color.g);
		test_lights.push_back(light.color.b);

		if (!light.shadow_map) continue;

		auto name = "shadow_map_" + to_string(shadow_map_index++);
		light.shadow_map->use(*program_, name.c_str(), texture_unit);
		program_->set_uniform_block("light_block", binding_point, assets.light_buffer);
	}

	static texture_buffer gpu_lights{usage::stream_draw, format::r32f};

	if (!assets.lights.empty())
		gpu_lights.bind_buffer_and_update(test_lights.size(), test_lights.data());
	else
		gpu_lights.bind_buffer_and_update<float>(1);

	gpu_lights.use(*program_, "lights", texture_unit);



#ifndef USE_TILED_SHADING
	program_->set_uniform("lights_size", static_cast<int>(assets.lights.size()));
#endif



	// Statics
	if (render_statics)
		for (auto entry : assets.statics)
		{
			const auto& entities = get<0>(entry);
			const auto& associated_models = get<1>(entry);

			for (auto entity : *entities)
			{
				const auto& model = *associated_models[entity.index];
				const auto& model_matrix = entity.transformation();

				// Uniforms
				program_->set_uniform("normal_matrix", 
					glm::inverseTranspose(glm::mat3(model_matrix)));
				program_->set_uniform("model_view_matrix", 
					camera->view_matrix * model_matrix);
				program_->set_uniform("model_view_projection_matrix", 
					camera->view_projection_matrix * model_matrix);

				model.render(*program_, texture_unit);
			}
		}

	// Screen-aligned quad
	if (render_screen_aligned_quad) {
		static screen_aligned_quad screen_aligned_quad;
		screen_aligned_quad.update(*camera);
		screen_aligned_quad.mesh_.render_without_material();
	}

	// Memory barrier
	glMemoryBarrier(post_memory_barrier_mask);
}

} // namespace renderer
} // namespace black_label