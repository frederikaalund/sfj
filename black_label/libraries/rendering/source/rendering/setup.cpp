#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/rendering/setup.hpp>

#include <stdexcept>

#include <GL/glew.h>



namespace black_label {
namespace rendering {

using namespace std;

setup::setup()
{
	glewExperimental = GL_TRUE;
	GLenum glew_error = glewInit();
  
	if (GLEW_OK != glew_error)
		throw runtime_error{"Glew failed to initialize."};

	if (!GLEW_VERSION_3_3)
		throw runtime_error{"Requires OpenGL version 3.3 or above."};

	GLint max_draw_buffers;
	glGetIntegerv(GL_MAX_DRAW_BUFFERS, &max_draw_buffers);
	if (3 > max_draw_buffers)
		throw runtime_error{"Requires at least 3 draw buffers."};

	if (!GLEW_ARB_shader_storage_buffer_object)
		throw runtime_error{"Requires GLEW_ARB_shader_storage_buffer_object."};

	if (!GLEW_ARB_shader_image_load_store)
		throw runtime_error{"Requires GLEW_ARB_shader_image_load_store."};

	// TODO: Refactor the following to somewhere else since it is not related to glew.
	if (GLEW_ARB_texture_storage)
		glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
	glEnable(GL_FRAMEBUFFER_SRGB);
	glClearColor(0.9f, 0.934f, 1.0f, 1.0f);
}


} // namespace rendering
} // namespace black_label