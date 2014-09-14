#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/renderer/glew_setup.hpp>

#include <stdexcept>

#include <GL/glew.h>



namespace black_label {
namespace renderer {

using namespace std;

glew_setup::glew_setup()
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
}


} // namespace renderer
} // namespace black_label