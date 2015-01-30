#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/rendering/pass.hpp>

#include <GL/glew.h>



using namespace std;
using namespace boost::adaptors;



namespace black_label {
namespace rendering {

using namespace gpu;



////////////////////////////////////////////////////////////////////////////////
/// Poisson Disc
////////////////////////////////////////////////////////////////////////////////
class poisson_disc
{
public:
	using data_type = std::vector<glm::vec2>;

	poisson_disc() : data(128)
	{ utility::generate_poisson_disc_samples(std::begin(data), std::end(data)); };

	operator data_type&() { return data; }

private:
	data_type data;
};



////////////////////////////////////////////////////////////////////////////////
/// Basic Pass
////////////////////////////////////////////////////////////////////////////////
void basic_pass::set_viewport( int width, int height ) const 
{ glViewport(0, 0, width, height); }

void basic_pass::set_blend_mode() const
{ glDisable(GL_BLEND); }

void basic_pass::set_depth_test() const {
	if (render_mode[render_mode::test_depth]) glEnable(GL_DEPTH_TEST);
	else glDisable(GL_DEPTH_TEST);
}

void basic_pass::set_face_culling_mode() const {
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
}

void basic_pass::set_clearing_mask() const
{ glClear(clearing_mask); }

void basic_pass::wait_for_opengl()
{
	// OpenGL Error checking
	assert(0 == glGetError());
	glFinish();
}



////////////////////////////////////////////////////////////////////////////////
/// Pass
////////////////////////////////////////////////////////////////////////////////
void pass::set_input_textures( unsigned int& texture_unit ) const {
	for (const auto& entry : input_textures) {
		const auto& texture_name = entry.first;
		const auto& texture = *entry.second;
		texture.use(*program, texture_name.c_str(), texture_unit);
	}
}

void pass::set_buffers( unsigned int& shader_storage_binding_point, unsigned int& uniform_binding_point ) const {
	for (const auto& entry : buffers) {
		const auto& buffer_name = entry.first;
		const auto& buffer = *entry.second;

		if ("photon_splatting" == name && buffer_name == "photon_buffer")
			continue;

		if (target::shader_storage == buffer.target)
			program->set_shader_storage_block(buffer_name, shader_storage_binding_point, buffer);
		else
			assert(false);
	}

	//for (const auto& buffer : index_bound_buffers | map_values | indirected)
	//	buffer.bind();

	
	for (const auto& entry : index_bound_buffers) {
		auto& name = entry.first;
		auto& buffer = *entry.second;

		if ("counter" != name) continue;

		buffer.bind();
		uint32_t count;
		glGetBufferSubData(buffer.target, 0, sizeof(uint32_t), &count);
		if (0 == count) {
			data_offsets->clear();
			data_offsets->emplace_back(count);
		} else {
			data_offsets->emplace_back(count + data_offsets->back()[0]);
		}
		program->set_uniform("total_data_offset", data_offsets->back()[0]);
		
		count = preincrement_buffer_counter;
		buffer.update(sizeof(uint32_t), &count);
	}
		
	static gpu::buffer gpu_data_offsets{gpu::target::shader_storage, gpu::usage::dynamic_draw};
	auto total_size = static_cast<ptrdiff_t>(sizeof(glm::uvec4) * data_offsets->size());
	gpu_data_offsets.bind_and_update(total_size, data_offsets->data());
	program->set_shader_storage_block("data_offset_block", shader_storage_binding_point, gpu_data_offsets);

}

void pass::set_uniforms() const {
	program->set_uniform("z_near", view->z_near);
	program->set_uniform("z_far", view->z_far);
	program->set_uniform("view_matrix", view->view_matrix);
	program->set_uniform("projection_matrix", view->projection_matrix);
	program->set_uniform("view_projection_matrix", view->view_projection_matrix);
	program->set_uniform("inverse_view_projection_matrix", glm::inverse(view->view_projection_matrix));
	program->set_uniform("wc_view_eye_position", view->eye);
	static poisson_disc poisson_disc;
	program->set_uniform("poisson_disc", poisson_disc);
	program->set_uniform("ldm_view_count", ldm_view_count);

	// Photon count
	auto& photon_count_buffer = boost::find_if(index_bound_buffers, [](const auto& entry) { return "photon_counter" == entry.first; });
	if (photon_count_buffer != end(index_bound_buffers)) {
		uint32_t photon_count;
		photon_count_buffer->second->bind();
		glGetBufferSubData(photon_count_buffer->second->target, 0, sizeof(uint32_t), &photon_count);
		program->set_uniform("photon_count", photon_count);
	}
}

void pass::set_auxiliary_views( unsigned int& shader_storage_binding_point, unsigned int& uniform_binding_point ) const {
	struct view_type {
		glm::mat4 view_matrix, projection_matrix, view_projection_matrix;
		glm::vec4 eye;
		glm::vec4 right, forward, up;
		glm::ivec2 dimensions;
		glm::vec2 padding;
	};
	static_assert(sizeof(float) * (16 * 3 + 4 * 4 + 2) + sizeof(int) * 2 == sizeof(view_type), "view_type must match the OpenGL GLSL layout(140) specification.");

	// Current view
	static gpu::buffer current_view_buffer{gpu::target::uniform_buffer, gpu::usage::dynamic_draw, sizeof(view_type)};
	view_type gpu_current_view;
	gpu_current_view.projection_matrix = view->projection_matrix;
	gpu_current_view.view_matrix = view->view_matrix;
	gpu_current_view.view_projection_matrix = view->view_projection_matrix;
	gpu_current_view.eye = glm::vec4(view->eye, 1.0);
	gpu_current_view.right = glm::vec4(view->right(), 1.0);
	gpu_current_view.forward = glm::vec4(view->forward(), 1.0);
	gpu_current_view.up = glm::vec4(view->up(), 1.0);
	gpu_current_view.dimensions = view->window;
	current_view_buffer.bind_and_update(sizeof(view_type), &gpu_current_view);
	program->set_uniform_block("current_view_block", uniform_binding_point, current_view_buffer);

	// User view
	static gpu::buffer user_view_buffer{gpu::target::uniform_buffer, gpu::usage::dynamic_draw, sizeof(view_type)};
	view_type gpu_user_view;
	gpu_user_view.projection_matrix = user_view->projection_matrix;
	gpu_user_view.view_matrix = user_view->view_matrix;
	gpu_user_view.view_projection_matrix = user_view->view_projection_matrix;
	gpu_user_view.dimensions = user_view->window;
	user_view_buffer.bind_and_update(sizeof(view_type), &gpu_user_view);
	program->set_uniform_block("user_view_block", uniform_binding_point, user_view_buffer);

	// Auxiliary views
	const auto total_size = static_cast<ptrdiff_t>(sizeof(view_type) * auxiliary_views.size());
	static gpu::buffer views_buffer{gpu::target::shader_storage, gpu::usage::dynamic_draw, total_size};
	std::vector<view_type> gpu_auxiliary_views;
	gpu_auxiliary_views.reserve(auxiliary_views.size());
	for (const auto& entry : auxiliary_views) {
		const auto& name = entry.first;
		const auto& view = *entry.second;

		view_type gpu_view;
		gpu_view.view_matrix = view.view_matrix;
		gpu_view.projection_matrix = view.projection_matrix;
		gpu_view.view_projection_matrix = view.view_projection_matrix;
		gpu_view.eye = glm::vec4(view.eye, 1.0);
		gpu_view.right = glm::vec4(view.right(), 1.0);
		gpu_view.forward = glm::vec4(view.forward(), 1.0);
		gpu_view.up = glm::vec4(view.up(), 1.0);
		gpu_view.dimensions = view.window;
		gpu_auxiliary_views.emplace_back(gpu_view);
	}

	views_buffer.bind_and_update(total_size, gpu_auxiliary_views.data());
	program->set_shader_storage_block("view_block", shader_storage_binding_point, views_buffer);
}

void pass::set_memory_barrier() const
{ glMemoryBarrier(post_memory_barrier_mask); }



void pass::render_photons(gpu::framebuffer& framebuffer, const black_label::rendering::view& view, unsigned int& texture_unit) const
{
	
	using namespace std;
	using namespace boost::adaptors;

	set_viewport(view, output_textures | map_values | indirected);
	set_blend_mode();
	set_depth_test();
	set_face_culling_mode();
	if (!set_framebuffer(framebuffer, output_textures | map_values | indirected)) {
		BOOST_LOG_TRIVIAL(warning) << "The framebuffer is not complete.";
		return;
	}
	set_clearing_mask();
	glClearColor(0.0, 0.0, 0.0, 0.0);

	program->set_uniform("view_matrix", view.view_matrix);
	program->set_uniform("view_projection_matrix", view.view_projection_matrix);

	auto& photon_count_buffer = boost::find_if(index_bound_buffers, [](const auto& entry) { return "photon_counter" == entry.first; })->second;
	uint32_t photon_count;
	photon_count_buffer->bind();
	glGetBufferSubData(photon_count_buffer->target, 0, sizeof(uint32_t), &photon_count);

	int stride{5 * 4 * sizeof(float)};

	auto& photon_buffer = boost::find_if(buffers, [](const auto& entry) { return "photon_buffer" == entry.first; })->second;
	glBindBuffer(photon_buffer->target, *photon_buffer);
	int32_t photon_buffer_size;
	glGetBufferParameteriv(photon_buffer->target, GL_BUFFER_SIZE, &photon_buffer_size);
	auto photons_to_use = std::min(static_cast<uint32_t>(photon_buffer_size / stride), photon_count);

	if (0 == photons_to_use) return;
	
	static auto vertex_array = gpu::vertex_array{generate};
	vertex_array.bind();
	gpu::vertex_array::index_type index{0};

	int arr_prev;
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &arr_prev);

	glBindBuffer(GL_ARRAY_BUFFER, *photon_buffer);
	
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);
	//glEnable(GL_POLYGON_OFFSET_FILL);
	//glEnable(GL_POLYGON_OFFSET_POINT);
	//glPolygonOffset(100.0f, 100.0f);
	glDepthMask(false);
	
	
	glVertexAttribPointer(index, 4, GL_FLOAT, GL_FALSE, stride, nullptr);
	glEnableVertexAttribArray(index++);
	glVertexAttribPointer(index, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(1 * 4 * sizeof(float)));
	glEnableVertexAttribArray(index++);
	glVertexAttribPointer(index, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(2 * 4 * sizeof(float)));
	glEnableVertexAttribArray(index++);
	glVertexAttribPointer(index, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(3 * 4 * sizeof(float)));
	glEnableVertexAttribArray(index++);
	glVertexAttribPointer(index, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(4 * 4 * sizeof(float)));
	glEnableVertexAttribArray(index++);

	glDrawArrays(GL_POINTS, 0, photons_to_use);
	
	glDisable(GL_BLEND);
	//glDisable(GL_POLYGON_OFFSET_FILL);
	//glDisable(GL_POLYGON_OFFSET_POINT);
	glDepthMask(true);

	// TODO: I don't have an explanation for why this is needed.
	glBindBuffer(GL_ARRAY_BUFFER, arr_prev);
}

} // namespace rendering
} // namespace black_label
