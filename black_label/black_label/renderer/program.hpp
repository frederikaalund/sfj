#ifndef BLACK_LABEL_RENDERER_PROGRAM_HPP
#define BLACK_LABEL_RENDERER_PROGRAM_HPP

#include <black_label/renderer/types_and_constants.hpp>
#include <black_label/shared_library/utility.hpp>

#include <bitset>

#include <glm/glm.hpp>



namespace black_label {
namespace renderer {

////////////////////////////////////////////////////////////////////////////////
/// Shader
////////////////////////////////////////////////////////////////////////////////
class shader
{
public:
	typedef unsigned int id_type;
	const static id_type invalid_id = 0;

	typedef unsigned int shader_type;

	typedef std::bitset<3> status_type;
	const static std::size_t 
		is_tried_instantiated_bit = 0,
		shader_file_found_bit = 1,
		compile_status_bit = 2;



	friend void swap( shader& rhs, shader& lhs )
	{
		using std::swap;
		swap(rhs.id, lhs.id);
		swap(rhs.status, lhs.status);
	}
	shader() : id(invalid_id) {}
	shader( shader&& other ) : id(invalid_id) { swap(*this, other); }
	shader( 
		shader_type type, 
		const char* path_to_shader, 
		const char* preprocessor_commands = "" );
	BLACK_LABEL_SHARED_LIBRARY ~shader();

	shader& operator =( shader lhs ) { swap(*this, lhs); return *this; }

	bool is_tried_instantiated() const
	{ return status.test(is_tried_instantiated_bit); }
	bool is_compiled() const
	{ return status.test(compile_status_bit); };
	bool is_shader_file_found() const
	{ return status.test(shader_file_found_bit); };
	bool is_complete() const
	{ return status.all(); };

	std::string get_info_log();

	id_type id;
	status_type status;



protected:
	shader( const shader& other );
};



////////////////////////////////////////////////////////////////////////////////
/// Core Program
////////////////////////////////////////////////////////////////////////////////
class core_program
{
public:
	typedef unsigned int id_type;
	const static id_type invalid_id = 0;

	friend void swap( core_program& rhs, core_program& lhs )
	{
		using std::swap;
		swap(rhs.id, lhs.id);
	}
	core_program() : id(invalid_id) {}
	core_program( core_program&& other ) : id(invalid_id) { swap(*this, other); }
	core_program( generate_type );
	~core_program();

	core_program& operator =( core_program lhs ) { swap(*this, lhs); return *this; }

	void use() const;
	void set_output_location( unsigned int color_number, const char* name );
	void link();

	unsigned int get_uniform_location( const char* name ) const;
	void set_uniform( const char* name, int value ) const;
	void set_uniform( const char* name, int value1, int value2 ) const;
	void set_uniform( const char* name, float value ) const;
	void set_uniform( const char* name, float value1, float value2, float value3 ) const;
	void set_uniform( const char* name, float value1, float value2, float value3, float value4 ) const;
	void set_uniform( const char* name, glm::vec2& value ) const;
	void set_uniform( const char* name, glm::vec3& value ) const;
	void set_uniform( const char* name, glm::vec4& value ) const;
	void set_uniform( const char* name, glm::mat3& value ) const;
	void set_uniform( const char* name, glm::mat4& value ) const;

	id_type id;



protected:
	core_program( const core_program& other );
};



////////////////////////////////////////////////////////////////////////////////
/// Program
////////////////////////////////////////////////////////////////////////////////
class program : public core_program
{
public:
	friend void swap( program& rhs, program& lhs )
	{
		using std::swap;
		swap(static_cast<core_program&>(rhs), static_cast<core_program&>(lhs));
		swap(rhs.vertex, lhs.vertex);
		swap(rhs.geometry, lhs.geometry);
		swap(rhs.fragment, lhs.fragment);
	}
	program() {}
	program( program&& other ) { swap(*this, other); }

	explicit program( 
		const char* path_to_vertex_shader,
		const char* path_to_geometry_shader = nullptr,
		const char* path_to_fragment_shader = nullptr,
		const char* preprocessor_commands = "")
		: core_program(black_label::renderer::generate)
	{
		setup(path_to_vertex_shader, path_to_geometry_shader, path_to_fragment_shader, preprocessor_commands);
		link();
	}

	template<typename iterator>
	explicit program( 
		const char* path_to_vertex_shader,
		const char* path_to_geometry_shader,
		const char* path_to_fragment_shader,
		const char* preprocessor_commands,
		iterator fragment_output_name_first,
		iterator fragment_output_name_last)
		: core_program(black_label::renderer::generate)
	{
		setup(path_to_vertex_shader, path_to_geometry_shader, path_to_fragment_shader, preprocessor_commands);
		set_output_locations(fragment_output_name_first, fragment_output_name_last);
		link();
	}

	BLACK_LABEL_SHARED_LIBRARY ~program();

	program& operator =( program rhs )
	{ swap(*this, rhs); return *this; }

	std::string get_info_log();
	std::string get_aggregated_info_log();

	shader vertex, geometry, fragment;



protected:
	program( const program& other );

	void setup(
		const char* path_to_vertex_shader,
		const char* path_to_geometry_shader = nullptr,
		const char* path_to_fragment_shader = nullptr,
		const char* preprocessor_commands = "");

	template<typename iterator>
	void set_output_locations(
		iterator fragment_output_name_first,
		iterator fragment_output_name_last)
	{
		unsigned int location = 0;
		std::for_each(fragment_output_name_first, fragment_output_name_last, 
			[&] ( const char* name ) { set_output_location(location++, name); });
	}
};

} // namespace renderer
} // namespace black_label



#endif