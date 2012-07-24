#ifndef BLACK_LABEL_RENDERER_STORAGE_MODEL_HPP
#define BLACK_LABEL_RENDERER_STORAGE_MODEL_HPP

#include <black_label/shared_library/utility.hpp>
#include <black_label/file_buffer.hpp>

#include <bitset>
#include <memory>



namespace black_label {
namespace renderer {
namespace storage {

struct color
{
	float r, g, b;
};

struct material 
{
	color diffuse;
	float alpha;
};

class model
{
public:
	typedef unsigned int render_mode_type;
	static const unsigned int invalid_vbo = 0;



	friend void swap( model& rhs, model& lhs )
	{
		using std::swap;
		swap(rhs.draw_count, lhs.draw_count);
		swap(rhs.vertex_vbo, lhs.vertex_vbo);
		swap(rhs.index_vbo, lhs.index_vbo);
		swap(rhs.normal_size, lhs.normal_size);
		swap(rhs.render_mode, lhs.render_mode);
	}

	model() : vertex_vbo(invalid_vbo) {}
	model( model&& other )
		: draw_count(other.draw_count) 
		, vertex_vbo(other.vertex_vbo) 
		, index_vbo(other.index_vbo) 
		, normal_size(other.normal_size) 
		, render_mode(other.render_mode)
	{ other.vertex_vbo = invalid_vbo; }

	model( 
		render_mode_type render_mode,
		const float* vertices_begin,
		const float* vertices_end,
		const float* normals_begin = nullptr,
		const unsigned int* indices_begin = nullptr,
		const unsigned int* indices_end = nullptr );

	BLACK_LABEL_SHARED_LIBRARY ~model();

	model& operator=( model lhs ) { swap(*this, lhs); return *this; }



	bool is_loaded() { return invalid_vbo != vertex_vbo; }
	bool has_indices() { return invalid_vbo != index_vbo; }

	void render();



	int draw_count;
	unsigned int vertex_vbo, index_vbo;
	int normal_size;
	unsigned int render_mode;
	material material;



protected:
	model( const model& other ); // Possible, but did not bother
};

class keyframed_model
{
public:
	keyframed_model( int capacity = 50 )
		: capacity(capacity)
		, size(0)
		, keyframes(new model[capacity]) {}

	bool is_loaded() { return 0 < size && keyframes[size-1].is_loaded(); }

	void push_back_keyframe( model&& model ) 
	{ keyframes[size++] = std::move(model); }

	void render( int keyframe ) { keyframes[keyframe].render(); };

	int capacity, size;
	std::unique_ptr<model[]> keyframes;
};



////////////////////////////////////////////////////////////////////////////////
/// Shader and Program
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

	void use();
	std::string get_info_log();
	std::string get_aggregated_info_log();

	shader vertex, geometry, fragment;
	id_type id;



protected:
	program( const program& other );
};

} // namespace storage
} // namespace renderer
} // namespace black_label



#endif
