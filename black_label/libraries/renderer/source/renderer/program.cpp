#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/renderer/program.hpp>

#include <black_label/file_buffer.hpp>

#include <sstream>

#include <boost/math/constants/constants.hpp>

#include <glm/gtc/type_ptr.hpp>

#include <GL/glew.h>
#ifndef APPLE
#include <GL/gl.h>
#else
#include <OpenGL/gl.h>
#endif



namespace black_label {
namespace renderer {

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
	
	static std::vector<char> numerical_constants;
	if (numerical_constants.empty())
	{
		std::stringstream numerical_constants_stream;
		numerical_constants_stream << "#define PI " << boost::math::constants::pi<float>() << std::endl;
		auto numerical_constants_string = numerical_constants_stream.str();
		numerical_constants.assign(numerical_constants_string.cbegin(), numerical_constants_string.cend());
		numerical_constants.push_back('\0');
	}

	const GLchar* source_code_data[] = { preprocessor_commands, numerical_constants.data(), source_code.data() };
	
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
/// Core Program
////////////////////////////////////////////////////////////////////////////////
core_program::core_program( generate_type ) : id(glCreateProgram()) {}

core_program::~core_program()
{
	if (id) glDeleteProgram(id);
}



void core_program::use() const { glUseProgram(id); }

void core_program::set_output_location( unsigned int location, const char* name )
{
	glBindFragDataLocation(id, location, name);
}

void core_program::set_attribute_location( unsigned int location, const char* name )
{
	glBindAttribLocation(id, location, name);
}
    
void core_program::link()
{
	glLinkProgram(id);
}



unsigned int core_program::get_uniform_location( const char* name ) const
{
	return glGetUniformLocation(id, name);
}


void core_program::set_uniform( const char* name, int value ) const
{
	auto location = get_uniform_location(name);
	assert(-1 != location);
	glUniform1i(location, value);
}

void core_program::set_uniform( const char* name, int value1, int value2 ) const
{
	auto location = get_uniform_location(name);
	assert(-1 != location);
	glUniform2i(location, value1, value2);
}

void core_program::set_uniform( const char* name, float value ) const
{
	auto location = get_uniform_location(name);
	assert(-1 != location);
	glUniform1f(location, value);
}

void core_program::set_uniform( const char* name, float value1, float value2, float value3 ) const
{
	auto location = get_uniform_location(name);
	assert(-1 != location);
	glUniform3f(location, value1, value2, value3);
}

void core_program::set_uniform( const char* name, float value1, float value2, float value3, float value4 ) const
{
	auto location = get_uniform_location(name);
	assert(-1 != location);
	glUniform4f(location, value1, value2, value3, value4);
}


void core_program::set_uniform( const char* name, const glm::vec2& value ) const
{
	auto location = get_uniform_location(name);
	assert(-1 != location);
	glUniform2fv(location, 1, glm::value_ptr(value));
}

void core_program::set_uniform( const char* name, const glm::vec3& value ) const
{
	auto location = get_uniform_location(name);
	assert(-1 != location);
	glUniform3fv(location, 1, glm::value_ptr(value));
}

void core_program::set_uniform( const char* name, const glm::vec4& value ) const
{
	auto location = get_uniform_location(name);
	assert(-1 != location);
	glUniform4fv(location, 1, glm::value_ptr(value));
}

void core_program::set_uniform( const char* name, const glm::mat3& value ) const
{
	auto location = get_uniform_location(name);
	assert(-1 != location);
	glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

void core_program::set_uniform( const char* name, const glm::mat4& value ) const
{
	auto location = get_uniform_location(name);
	assert(-1 != location);
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
}



////////////////////////////////////////////////////////////////////////////////
/// Program
////////////////////////////////////////////////////////////////////////////////
BLACK_LABEL_SHARED_LIBRARY program::~program()
{
	if (!id) return;

	if (vertex.is_complete()) glDetachShader(id, vertex.id);
	if (geometry.is_complete()) glDetachShader(id, geometry.id);
	if (fragment.is_complete()) glDetachShader(id, fragment.id);
}



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

void program::setup(
	const char* path_to_vertex_shader,
	const char* path_to_geometry_shader,
	const char* path_to_fragment_shader,
	const char* preprocessor_commands)
{
	vertex = shader(GL_VERTEX_SHADER, path_to_vertex_shader, preprocessor_commands);
	if (path_to_geometry_shader)
		geometry = shader(GL_GEOMETRY_SHADER, path_to_geometry_shader, preprocessor_commands);
	if (path_to_fragment_shader)
		fragment = shader(GL_FRAGMENT_SHADER, path_to_fragment_shader, preprocessor_commands);

	if (vertex.is_complete()) glAttachShader(id, vertex.id);
	if (geometry.is_complete()) glAttachShader(id, geometry.id);
	if (fragment.is_complete()) glAttachShader(id, fragment.id);
}

} // namespace renderer
} // namespace black_label
