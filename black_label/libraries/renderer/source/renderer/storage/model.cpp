#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/renderer/storage/model.hpp>

#include <cassert>
#include <string>

#include <GL/glew.h>

using namespace std;



namespace black_label {
namespace renderer {
namespace storage {


model::model( 
	render_mode_type render_mode, 
	const float* vertices_begin,
	const float* vertices_end,
	const float* normals_begin,
	const unsigned int* indices_begin,
	const unsigned int* indices_end )
	: render_mode(render_mode)
{
////////////////////////////////////////////////////////////////////////////////
/// Vertices
////////////////////////////////////////////////////////////////////////////////
	draw_count = vertices_end-vertices_begin;
	GLsizeiptr attribute_size = draw_count*sizeof(float);
	GLsizeiptr total_size = attribute_size;
	if (nullptr == normals_begin)
		normal_size = 0;
	else
	{
		normal_size = attribute_size;
		total_size *= 2;
	}

	glGenBuffers(1, &vertex_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_vbo);
	glBufferData(
		GL_ARRAY_BUFFER, 
		total_size,
		nullptr,
		GL_STATIC_DRAW);

	glBufferSubData(GL_ARRAY_BUFFER, 0, attribute_size, vertices_begin);
	if (normal_size)
		glBufferSubData(GL_ARRAY_BUFFER, attribute_size, normal_size, normals_begin);



////////////////////////////////////////////////////////////////////////////////
/// Indices
////////////////////////////////////////////////////////////////////////////////
	if (indices_begin == indices_end)
	{
		index_vbo = invalid_vbo;

		switch (render_mode)
		{
		case GL_LINES:
			{
				// (2 indices)/(6 floats) = 1/3 indices/float
				draw_count /= 3;
			}
			break;
		case GL_QUADS:
			{
				// (4 indices)/(12 floats) = 1/3 indices/float
				draw_count /= 3;
			}
			break;
		default:
			{
				assert(false);
			}
		}

		return;
	}

	draw_count = indices_end-indices_begin;
	glGenBuffers(1, &index_vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_vbo);
	glBufferData(
		GL_ELEMENT_ARRAY_BUFFER, 
		draw_count*sizeof(unsigned int),
		indices_begin,
		GL_STATIC_DRAW);
}

model::~model()
{
	if (!is_loaded()) return;

	glDeleteBuffers(1, &vertex_vbo);
	if (has_indices()) glDeleteBuffers(1, &index_vbo);
}



void model::render()
{
	glBindBuffer(GL_ARRAY_BUFFER, vertex_vbo);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, 0);
	if (normal_size)
	{
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, 0, reinterpret_cast<GLvoid*>(normal_size));
	}

	if (has_indices())
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_vbo);
		glDrawElements(render_mode, draw_count, GL_UNSIGNED_INT, 0);
	}
	else
		glDrawArrays(render_mode, 0, draw_count);
	
	glDisableClientState(GL_VERTEX_ARRAY);
}



////////////////////////////////////////////////////////////////////////////////
/// Shader
////////////////////////////////////////////////////////////////////////////////
shader::shader( shader_type type, const char* path_to_shader )
{
	status.set(is_tried_instantiated_bit);

	file_buffer::file_buffer 
		source_code(path_to_shader, file_buffer::null_terminated);
	
	if (!source_code.data())
	{
		id = invalid_id;
		return;
	}
	status.set(shader_file_found_bit);
	
	const GLchar* source_code_data = source_code.data();
	
	id = glCreateShader(type);
	glShaderSource(id, 1, &source_code_data, nullptr);
	glCompileShader(id);
	
	GLint compile_status;
	glGetShaderiv(id, GL_COMPILE_STATUS, &compile_status);
	if (compile_status)
		status.set(compile_status_bit);
}

BLACK_LABEL_SHARED_LIBRARY shader::~shader() 
{ 
	if (id) glDeleteShader(id);
}

string shader::get_info_log()
{
	string result;
	GLint info_log_length;
	glGetShaderiv(id, GL_INFO_LOG_LENGTH, &info_log_length);

	if (0 < info_log_length)
	{
		result.resize(info_log_length);
		glGetShaderInfoLog(id, info_log_length, nullptr, &result[0]);
		result.erase(result.find_last_not_of('\0')+1);
	}

	return result;
}



program::program( 
	const char* path_to_vertex_shader, 
	const char* path_to_geometry_shader, 
	const char* path_to_fragment_shader )
	: vertex(GL_VERTEX_SHADER, path_to_vertex_shader)
{
	id = glCreateProgram();

	if (path_to_geometry_shader)
		geometry = shader(GL_GEOMETRY_SHADER, path_to_geometry_shader);
	if (path_to_fragment_shader)
		fragment = shader(GL_FRAGMENT_SHADER, path_to_fragment_shader);

	if (vertex.is_complete()) glAttachShader(id, vertex.id);
	if (geometry.is_complete()) glAttachShader(id, geometry.id);
	if (fragment.is_complete()) glAttachShader(id, fragment.id);

	glLinkProgram(id);
}

BLACK_LABEL_SHARED_LIBRARY program::~program()
{
	if (id)
	{
		if (vertex.is_complete()) glDetachShader(id, vertex.id);
		if (geometry.is_complete()) glDetachShader(id, geometry.id);
		if (fragment.is_complete()) glDetachShader(id, fragment.id);
		glDeleteProgram(id);
	}
}

void program::use() { glUseProgram(id); }

std::string program::get_info_log()
{
	string result;
	GLint info_log_length;
	glGetProgramiv(id, GL_INFO_LOG_LENGTH, &info_log_length);

	if (0 < info_log_length)
	{
		result.resize(info_log_length);
		glGetProgramInfoLog(id, info_log_length, nullptr, &result[0]);
		result.erase(result.find_last_not_of('\0')+1);
	}

	return result;
}

std::string program::get_aggregated_info_log()
{
	string result;

	// Vertex
	if (vertex.is_shader_file_found())
		result += vertex.get_info_log();
	else
		result += "Vertex shader file was not found.\n";

	// Geometry
	if (geometry.is_tried_instantiated())
	{
		if (geometry.is_shader_file_found())
			result += geometry.get_info_log();
		else
			result += "Geometry shader file was not found.\n";
	}

	// Fragment
	if (fragment.is_tried_instantiated())
	{
		if (fragment.is_shader_file_found())
			result += fragment.get_info_log();
		else
			result += "Fragment shader file was not found.\n";
	}

	// Program
	result += get_info_log();

	return result;
}

} // namespace storage
} // namespace renderer
} // namespace black_label