#ifndef BLACK_LABEL_RENDERER_PROGRAM_HPP
#define BLACK_LABEL_RENDERER_PROGRAM_HPP

#include <black_label/path.hpp>
#include <black_label/renderer/gpu/buffer.hpp>
#include <black_label/renderer/types_and_constants.hpp>
#include <black_label/shared_library/utility.hpp>

#include <array>
#include <bitset>
#include <iterator>
#include <vector>

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
	const static id_type invalid_id{0};

	typedef unsigned int shader_type;

	typedef std::bitset<3> status_type;
	const static std::size_t 
		is_tried_instantiated_bit{0},
		shader_file_found_bit{1},
		compile_status_bit{2};



	friend void swap( shader& rhs, shader& lhs )
	{
		using std::swap;
		swap(rhs.id, lhs.id);
		swap(rhs.status, lhs.status);
		swap(rhs.type, lhs.type);
		swap(rhs.preprocessor_commands, lhs.preprocessor_commands);
		swap(rhs.path_to_shader, lhs.path_to_shader);
	}
	shader() : id(invalid_id) {}
	shader( shader&& other ) : id(invalid_id) { swap(*this, other); }
	shader( 
		shader_type type, 
		const path& path_to_shader, 
		const std::string& preprocessor_commands = std::string() );
	BLACK_LABEL_SHARED_LIBRARY ~shader();

	shader& operator =( shader lhs ) { swap(*this, lhs); return *this; }

	void load();

	bool is_tried_instantiated() const
	{ return status.test(is_tried_instantiated_bit); }
	bool is_compiled() const
	{ return status.test(compile_status_bit); };
	bool is_shader_file_found() const
	{ return status.test(shader_file_found_bit); };
	bool is_complete() const
	{ return status.all(); };

	std::string get_info_log() const;

	id_type id;
	status_type status;

	shader_type type;
	std::string preprocessor_commands;
	path path_to_shader;



protected:
	shader( const shader& other );
};



////////////////////////////////////////////////////////////////////////////////
/// Interface
////////////////////////////////////////////////////////////////////////////////
namespace interface {
	using type = unsigned int;
	extern const type shader_storage_block;
} // namespace interface


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
	void set_output_location( unsigned int location, const std::string& name );
	void set_attribute_location( unsigned int location, const std::string& name );
	void link() const;

	unsigned int get_uniform_location( const std::string& name ) const;
	unsigned int get_uniform_block_index( const std::string& name ) const;
	unsigned int get_resource_index( interface::type interface, const std::string& name ) const;

	template<typename... T>
	bool set_uniform( const std::string& name, T&&... values ) const
	{
		auto location = get_uniform_location(name);
		set_uniform(location, std::forward<T>(values)...);
		return -1 != location;
	}
	void set_uniform( unsigned int location, int value ) const;
	void set_uniform( unsigned int location, int value1, int value2 ) const;
	void set_uniform( unsigned int location, float value ) const;
	void set_uniform( unsigned int location, float value1, float value2, float value3 ) const;
	void set_uniform( unsigned int location, float value1, float value2, float value3, float value4 ) const;
	void set_uniform( unsigned int location, const glm::vec2& value ) const;
	void set_uniform( unsigned int location, const glm::vec3& value ) const;
	void set_uniform( unsigned int location, const glm::vec4& value ) const;
	void set_uniform( unsigned int location, const glm::mat3& value ) const;
	void set_uniform( unsigned int location, const glm::mat4& value ) const;
	void set_uniform( unsigned int location, const std::vector<glm::vec2>& value ) const;

	template<typename... T>
	bool set_uniform_block( const std::string& name, unsigned int& binding_point, T&&... values ) const
	{
		auto index = get_uniform_block_index(name);
		if (-1 != index)
			set_uniform_block(index, binding_point, std::forward<T>(values)...);
		return -1 != index;
	}
	void set_uniform_block( unsigned int index, unsigned int& binding_point, const gpu::buffer& value ) const;

	template<typename... T>
	bool set_shader_storage_block( const std::string& name, unsigned int& binding_point, T&&... values ) const
	{
		auto index = get_resource_index(interface::shader_storage_block, name);
		if (-1 != index)
			set_shader_storage_block(index, binding_point, std::forward<T>(values)...);
		return -1 != index;
	}
	void set_shader_storage_block( unsigned int index, unsigned int& binding_point, const gpu::buffer& value ) const;

	id_type id;



protected:
	core_program( const core_program& other );

	unsigned int get_uniform_location_checked( const std::string& name ) const;
};



////////////////////////////////////////////////////////////////////////////////
/// Program
////////////////////////////////////////////////////////////////////////////////
class program : public core_program
{
public:

////////////////////////////////////////////////////////////////////////////////
/// Configuration
////////////////////////////////////////////////////////////////////////////////
	class configuration
	{
	public:
		friend class program;

		typedef std::string name_type;

		configuration()	{}


		configuration& vertex_shader( const path& value ) 
		{ path_to_vertex_shader_ = value; return *this; }

		configuration& geometry_shader( const path& value ) 
		{ path_to_geometry_shader_ = value; return *this; }

		configuration& fragment_shader( const path& value ) 
		{ path_to_fragment_shader_ = value; return *this; }

		configuration& preprocessor_commands( const std::string& value ) 
		{ preprocessor_commands_ = value; return *this; }


		configuration& add_vertex_attribute( const name_type& value ) 
		{ vertex_attribute_names_.push_back(value); return *this; }

		configuration& add_fragment_output( const name_type& value ) 
		{ fragment_output_names_.push_back(value); return *this; }


		configuration& shader_directory( const path& shader_directory )
		{
			if (!path_to_vertex_shader_.empty())
				canonical_and_preferred(path_to_vertex_shader_, shader_directory);
			if (!path_to_geometry_shader_.empty())
				canonical_and_preferred(path_to_geometry_shader_, shader_directory);
			if (!path_to_fragment_shader_.empty())
				canonical_and_preferred(path_to_fragment_shader_, shader_directory);

			return *this;
		}



		path 
			path_to_vertex_shader_, 
			path_to_geometry_shader_, 
			path_to_fragment_shader_;
		std::string preprocessor_commands_;
		std::vector<name_type> 
			vertex_attribute_names_,
			fragment_output_names_;
	};



	friend void swap( program& rhs, program& lhs )
	{
		using std::swap;
		swap(static_cast<core_program&>(rhs), static_cast<core_program&>(lhs));
		swap(rhs.vertex, lhs.vertex);
		swap(rhs.geometry, lhs.geometry);
		swap(rhs.fragment, lhs.fragment);
	}
	program() {}
	program( const program& ) = delete;
	program( program&& other ) : program{} { swap(*this, other); }

	program( const configuration& configuration )
		: core_program{black_label::renderer::generate}
	{
		setup(
			configuration.path_to_vertex_shader_, 
			configuration.path_to_geometry_shader_, 
			configuration.path_to_fragment_shader_, 
			configuration.preprocessor_commands_);
		set_output_locations(
			configuration.fragment_output_names_.cbegin(),
			configuration.fragment_output_names_.cend());
		set_attribute_locations(
			configuration.vertex_attribute_names_.cbegin(),
			configuration.vertex_attribute_names_.cend());
		link();
	}

	BLACK_LABEL_SHARED_LIBRARY ~program();

	program& operator=( program rhs ) { swap(*this, rhs); return *this; }

	bool is_complete() const;
	void reload( path program_file );
	std::string get_info_log() const;
	std::string get_aggregated_info_log() const;

	shader vertex, geometry, fragment;



protected:
	void setup(
		const path& path_to_vertex_shader,
		const path& path_to_geometry_shader = path(),
		const path& path_to_fragment_shader = path(),
		const std::string& preprocessor_commands = std::string());

	template<typename iterator>
	void set_output_locations(
		iterator fragment_output_name_first,
		iterator fragment_output_name_last)
	{
		unsigned int location = 0;
		std::for_each(fragment_output_name_first, fragment_output_name_last,
			[&] ( const std::string& name ) { set_output_location(location++, name); });
	}
    
	template<typename iterator>
	void set_attribute_locations(
        iterator vertex_attribute_name_first,
        iterator vertex_attribute_name_last)
	{
		unsigned int location = 0;		
		std::for_each(vertex_attribute_name_first, vertex_attribute_name_last,
			[&] ( const std::string& name ) { set_attribute_location(location++, name); });
	}
};

} // namespace renderer
} // namespace black_label



#endif