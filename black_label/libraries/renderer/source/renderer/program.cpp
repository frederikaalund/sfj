#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/renderer/program.hpp>

#include <black_label/file_buffer.hpp>

#include <sstream>

#include <boost/math/constants/constants.hpp>

#include <GL/glew.h>
#ifndef APPLE
#include <GL/gl.h>
#else
#include <OpenGL/gl.h>
#endif


namespace black_label
{
namespace renderer
{

using std::string;



////////////////////////////////////////////////////////////////////////////////
/// Shader
////////////////////////////////////////////////////////////////////////////////
shader::shader( 
	shader_type type, 
	const char* path_to_shader, 
	const char* preprocessor_commands )
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
	
	static const char* numerical_constants = nullptr;
	if (!numerical_constants)
	{
		std::stringstream numerical_constants_stream;
		numerical_constants_stream << "#define PI " << boost::math::constants::pi<float>() << std::endl;
		const char* numerical_constants = numerical_constants_stream.str().c_str();
	}

	const GLchar* source_code_data[] = { preprocessor_commands, numerical_constants, source_code.data() };
	
	id = glCreateShader(type);
	glShaderSource(id, 3, source_code_data, nullptr);
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



////////////////////////////////////////////////////////////////////////////////
/// Program
////////////////////////////////////////////////////////////////////////////////
program::program( 
	const char* path_to_vertex_shader, 
	const char* path_to_geometry_shader, 
	const char* path_to_fragment_shader,
	const char* preprocessor_commands )
	: vertex(GL_VERTEX_SHADER, path_to_vertex_shader, preprocessor_commands)
{
	id = glCreateProgram();

	if (path_to_geometry_shader)
		geometry = shader(GL_GEOMETRY_SHADER, path_to_geometry_shader, preprocessor_commands);
	if (path_to_fragment_shader)
		fragment = shader(GL_FRAGMENT_SHADER, path_to_fragment_shader, preprocessor_commands);

	if (vertex.is_complete()) glAttachShader(id, vertex.id);
	if (geometry.is_complete()) glAttachShader(id, geometry.id);
	if (fragment.is_complete()) glAttachShader(id, fragment.id);

	glBindAttribLocation(id, 0, "position");
	glBindAttribLocation(id, 1, "normal");
	glBindAttribLocation(id, 2, "texture_coordinates");
	glBindAttribLocation(id, 3, "camera_ray_direction");

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

void program::use() const { glUseProgram(id); }

std::string program::get_info_log()
{
	string result;
	GLint info_log_length;
	glGetProgramiv(id, GL_INFO_LOG_LENGTH, &info_log_length);

	if (0 < info_log_length)
	{
		result.resize(info_log_length);
		glGetProgramInfoLog(id, info_log_length, nullptr, &result[0]);
		result.erase(result.find_last_not_of('\0') + 1);
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

} // namespace renderer
} // namespace black_label
