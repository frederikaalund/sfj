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

void pass::set_buffers( unsigned int& shader_storage_binding_point ) const {
	for (const auto& entry : buffers) {
		const auto& buffer_name = entry.first;
		const auto& buffer = *entry.second;

		if (target::shader_storage == buffer.target)
			program->set_shader_storage_block(buffer_name, shader_storage_binding_point, buffer);
		else
			assert(false);
	}

	for (const auto& buffer : index_bound_buffers | map_values | indirected)
		buffer.bind();
}

void pass::set_uniforms() const
{
	program->set_uniform("z_near", view->z_near);
	program->set_uniform("z_far", view->z_far);
	program->set_uniform("view_matrix", view->view_matrix);
	program->set_uniform("projection_matrix", view->projection_matrix);
	program->set_uniform("view_projection_matrix", view->view_projection_matrix);
	program->set_uniform("inverse_view_projection_matrix", glm::inverse(view->view_projection_matrix));
	program->set_uniform("wc_view_eye_position", view->eye);
	static poisson_disc poisson_disc;
	program->set_uniform("poisson_disc", poisson_disc);
}

void pass::set_memory_barrier() const
{ glMemoryBarrier(post_memory_barrier_mask); }

} // namespace rendering
} // namespace black_label
