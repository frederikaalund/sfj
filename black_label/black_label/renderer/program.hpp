#ifndef BLACK_LABEL_RENDERER_PROGRAM_HPP
#define BLACK_LABEL_RENDERER_PROGRAM_HPP

#include <black_label/shared_library/utility.hpp>

#include <bitset>



namespace black_label
{
namespace renderer
{

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
	shader( shader_type type, const char* path_to_shader );
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
/// Program
////////////////////////////////////////////////////////////////////////////////
class program
{
public:
	typedef unsigned int id_type;
	const static id_type invalid_id = 0;

	friend void swap( program& rhs, program& lhs )
	{
		using std::swap;
		swap(rhs.vertex, lhs.vertex);
		swap(rhs.geometry, lhs.geometry);
		swap(rhs.fragment, lhs.fragment);
		swap(rhs.id, lhs.id);
	}
	program() : id(invalid_id) {}
	program( program&& other ) : id(invalid_id) { swap(*this, other); }
	explicit program( 
		const char* path_to_vertex_shader,
		const char* path_to_geometry_shader = nullptr,
		const char* path_to_fragment_shader = nullptr );
	BLACK_LABEL_SHARED_LIBRARY ~program();

	program& operator =( program lhs ) { swap(*this, lhs); return *this; }

	void use() const;
	std::string get_info_log();
	std::string get_aggregated_info_log();

	shader vertex, geometry, fragment;
	id_type id;



protected:
	program( const program& other );
};

} // namespace renderer
} // namespace black_label



#endif