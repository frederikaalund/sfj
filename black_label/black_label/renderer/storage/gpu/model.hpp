#ifndef BLACK_LABEL_RENDERER_STORAGE_GPU_MODEL_HPP
#define BLACK_LABEL_RENDERER_STORAGE_GPU_MODEL_HPP

#include <black_label/file_buffer.hpp>
#include <black_label/renderer/light.hpp>
#include <black_label/renderer/material.hpp>
#include <black_label/renderer/program.hpp>
#include <black_label/renderer/storage/cpu/model.hpp>
#include <black_label/renderer/storage/gpu/texture.hpp>
#include <black_label/renderer/storage/gpu/vertex_array.hpp>
#include <black_label/shared_library/utility.hpp>
#include <black_label/utility/algorithm.hpp>

#include <algorithm>
#include <bitset>
#include <memory>
#include <istream>
#include <ostream>

#include <boost/crc.hpp>
#include <boost/serialization/access.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>



namespace black_label {
namespace renderer {

class renderer;

namespace storage {
namespace gpu {

////////////////////////////////////////////////////////////////////////////////
/// Mesh
////////////////////////////////////////////////////////////////////////////////
typedef unsigned int render_mode_type;

class mesh
{
public:
	friend class boost::serialization::access;

	typedef buffer<target::array, usage::static_draw> vertex_buffer_type;
	typedef buffer<target::element_array, usage::static_draw> index_buffer_type;



	friend void swap( mesh& rhs, mesh& lhs )
	{
		using std::swap;
		swap(rhs.draw_count, lhs.draw_count);
		swap(rhs.vertex_buffer, lhs.vertex_buffer);
		swap(rhs.index_buffer, lhs.index_buffer);
		swap(rhs.vertex_array, lhs.vertex_array);
		swap(rhs.render_mode, lhs.render_mode);
		swap(rhs.material, lhs.material);
		swap(rhs.diffuse_texture, lhs.diffuse_texture);
		swap(rhs.specular_texture, lhs.specular_texture);
	}

	mesh() {}
	mesh( mesh&& other ) { swap(*this, other); }

	mesh( const cpu::mesh& cpu_mesh );
	mesh(
		const material& material,
		render_mode_type render_mode,
		const float* vertices_begin,
		const float* vertices_end,
		const float* normals_begin = nullptr,
		const float* texture_coordinates_begin = nullptr,
		const unsigned int* indices_begin = nullptr,
		const unsigned int* indices_end = nullptr );

	mesh& operator=( mesh lhs ) { swap(*this, lhs); return *this; }

	void load(
		const float* vertices_begin,
		const float* vertices_end,
		const float* normals_begin = nullptr,
		const float* texture_coordinates_begin = nullptr,
		const unsigned int* indices_begin = nullptr,
		const unsigned int* indices_end = nullptr );
	bool is_loaded() const { return vertex_buffer.valid(); }
	bool has_indices() const { return index_buffer.valid(); }

	void render( const core_program& program, int& texture_unit ) const;
	void render_without_material() const;



	template<typename archive_type>
	void serialize( archive_type& archive, unsigned int version )
	{
		cpu::mesh::vector_container vertices, normals, texture_coordinates;
		cpu::mesh::index_container indices;
		archive & vertices & normals & texture_coordinates & indices
			& render_mode & material;

		load(
			vertices.data(),
			&vertices[vertices.capacity()],
			(!normals.empty()) ? normals.data() : nullptr,
			(!texture_coordinates.empty()) ? texture_coordinates.data() : nullptr,
			(!indices.empty()) ? indices.data() : nullptr,
			(!indices.empty()) ? &indices[indices.capacity()] : nullptr);
	}



	int draw_count;
	vertex_buffer_type vertex_buffer;
	index_buffer_type index_buffer;
	vertex_array vertex_array;
	render_mode_type render_mode;
	material material;
	texture_2d diffuse_texture, specular_texture;



protected:
	mesh( const mesh& other ); // Possible, but did not bother
};



class keyframed_mesh
{
public:
	keyframed_mesh( int capacity = 50 )
		: capacity(capacity)
		, size(0)
		, keyframes(new mesh[capacity]) {}

	bool is_loaded() { return 0 < size && keyframes[size-1].is_loaded(); }

	void push_back_keyframe( mesh&& mesh ) 
	{ keyframes[size++] = std::move(mesh); }

	//void render( int keyframe, program::id_type program_id ) { keyframes[keyframe].render(program_id); };

	int capacity, size;
	std::unique_ptr<mesh[]> keyframes;
};



////////////////////////////////////////////////////////////////////////////////
/// Model
////////////////////////////////////////////////////////////////////////////////
typedef boost::crc_32_type::value_type checksum_type;

class model
{
public:
	typedef cpu::model::light_container light_container;

	friend class renderer;
	friend class boost::serialization::access;



	friend void swap( model& lhs, model& rhs )
	{
		using std::swap;
		swap(lhs.meshes, rhs.meshes);
		swap(lhs.model_file_checksum_, rhs.model_file_checksum_);
	}

	model() {}
	model( model&& other ) { swap(*this, other); }
	model( checksum_type model_file_checksum ) 
		: model_file_checksum_(model_file_checksum) {}

	model& operator=( model rhs ) { swap(*this, rhs); return *this; }

	bool is_loaded() const { return !meshes.empty(); }
	bool has_lights() const { return !lights.empty(); }
	checksum_type model_file_checksum() const { return model_file_checksum_; }

	void add_light( const light& light ) { lights.push_back(light); }
	void push_back( mesh&& mesh ) { meshes.push_back(std::move(mesh)); }
	void render( const core_program& program, int& texture_unit ) const
	{
		std::for_each(meshes.cbegin(), meshes.cend(), 
			[&] ( const mesh& mesh ){ mesh.render(program, texture_unit); });
	}
	void render_without_material() const
	{
		std::for_each(meshes.cbegin(), meshes.cend(), 
			[&] ( const mesh& mesh ){ mesh.render_without_material(); });
	}

	void clear()
	{ 
		std::generate(meshes.begin(), meshes.end(), [](){ return mesh(); });
		lights.clear();
	}

	template<typename char_type>
	static checksum_type peek_model_file_checksum( std::basic_istream<char_type>& stream )
	{
		checksum_type checksum;
		typename std::basic_istream<char_type>::pos_type position_before = stream.tellg();
		stream.seekg(typename std::basic_istream<char_type>::pos_type(0));
		stream.read(reinterpret_cast<char_type*>(&checksum), sizeof(checksum_type));
		stream.seekg(position_before);
		return checksum;
	}



	template<typename archive_type>
	void serialize( archive_type& archive, unsigned int version )
	{
		archive.load_binary(&model_file_checksum_, sizeof(checksum_type));
		archive & meshes & lights;
	}



protected:
	model( const model& other ); // Possible, but did not bother

private:
	std::vector<mesh> meshes;
	light_container lights;
	checksum_type model_file_checksum_;
};

} // namespace gpu
} // namespace storage
} // namespace renderer
} // namespace black_label



#endif
