#ifndef BLACK_LABEL_RENDERER_STORAGE_GPU_MODEL_HPP
#define BLACK_LABEL_RENDERER_STORAGE_GPU_MODEL_HPP

#include <black_label/file_buffer.hpp>
#include <black_label/renderer/light.hpp>
#include <black_label/renderer/material.hpp>
#include <black_label/renderer/storage/cpu/model.hpp>
#include <black_label/shared_library/utility.hpp>
#include <black_label/utility/algorithm.hpp>

#include <algorithm>
#include <array>
#include <bitset>
#include <memory>
#include <istream>
#include <ostream>

#include <boost/crc.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>



namespace black_label
{
namespace renderer 
{

class renderer;

namespace storage 
{
namespace gpu
{

////////////////////////////////////////////////////////////////////////////////
/// Mesh
////////////////////////////////////////////////////////////////////////////////
typedef unsigned int render_mode_type;

class mesh
{
public:
	static const unsigned int invalid_vbo = 0;



	friend void swap( mesh& rhs, mesh& lhs )
	{
		using std::swap;
		swap(rhs.draw_count, lhs.draw_count);
		swap(rhs.vertex_vbo, lhs.vertex_vbo);
		swap(rhs.index_vbo, lhs.index_vbo);
		swap(rhs.normal_size, lhs.normal_size);
		swap(rhs.render_mode, lhs.render_mode);
		swap(rhs.material, lhs.material);
	}

	mesh() : vertex_vbo(invalid_vbo) {}
	mesh( mesh&& other ) : vertex_vbo(invalid_vbo) { swap(*this, other); }

	mesh( const cpu::mesh& cpu_mesh );
	mesh(
		const material& material,
		render_mode_type render_mode,
		const float* vertices_begin,
		const float* vertices_end,
		const float* normals_begin = nullptr,
		const unsigned int* indices_begin = nullptr,
		const unsigned int* indices_end = nullptr );

	BLACK_LABEL_SHARED_LIBRARY ~mesh();

	mesh& operator=( mesh lhs ) { swap(*this, lhs); return *this; }

	void load(
		const float* vertices_begin,
		const float* vertices_end,
		const float* normals_begin = nullptr,
		const unsigned int* indices_begin = nullptr,
		const unsigned int* indices_end = nullptr );
	bool is_loaded() const { return invalid_vbo != vertex_vbo; }
	bool has_indices() const { return invalid_vbo != index_vbo; }

	void render() const;

	template<typename char_type>
	friend std::basic_istream<char_type>& operator>>( 
		std::basic_istream<char_type>& stream, 
		mesh& mesh )
	{
		cpu::mesh::vector_container::size_type 
			vertices_capacity,
			normals_capacity;
		cpu::mesh::index_container::size_type
			indices_capacity;

		stream.read(reinterpret_cast<char_type*>(&mesh.render_mode), sizeof(render_mode_type));
		stream.read(reinterpret_cast<char_type*>(&vertices_capacity), sizeof(cpu::mesh::vector_container::size_type));
		stream.read(reinterpret_cast<char_type*>(&normals_capacity), sizeof(cpu::mesh::vector_container::size_type));
		stream.read(reinterpret_cast<char_type*>(&indices_capacity), sizeof(cpu::mesh::index_container::size_type));

		cpu::mesh::vector_container 
			vertices(vertices_capacity),
			normals(normals_capacity);
		cpu::mesh::index_container
			indices(indices_capacity);

		stream.read(reinterpret_cast<char_type*>(vertices.data()), vertices.capacity() * sizeof(cpu::mesh::vector_container::value_type));
		if (!normals.empty())
			stream.read(reinterpret_cast<char_type*>(normals.data()), normals.capacity() * sizeof(cpu::mesh::vector_container::value_type));
		if (!indices.empty())
			stream.read(reinterpret_cast<char_type*>(indices.data()), indices.capacity() * sizeof(cpu::mesh::index_container::value_type));

		stream >> mesh.material;

		mesh.load(
			vertices.data(), 
			&vertices[vertices.capacity()],
			(!normals.empty()) ? normals.data() : nullptr,
			(!indices.empty()) ? indices.data() : nullptr,
			(!indices.empty()) ? &indices[indices.capacity()] : nullptr);

		return stream;
	}



	int draw_count;
	unsigned int vertex_vbo, index_vbo;
	int normal_size;
	render_mode_type render_mode;
	material material;



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

	void render( int keyframe ) { keyframes[keyframe].render(); };

	int capacity, size;
	std::unique_ptr<mesh[]> keyframes;
};



////////////////////////////////////////////////////////////////////////////////
/// Model
////////////////////////////////////////////////////////////////////////////////
#define BLACK_LABEL_RENDERER_STORAGE_MODEL_MESHES_MAX 12
typedef boost::crc_32_type::value_type checksum_type;

class model
{
public:
	typedef cpu::model::light_container light_container;

	friend class renderer;

	friend void swap( model& lhs, model& rhs )
	{
		using std::swap;
    
    auto lhs_i = lhs.meshes.begin();
    auto rhs_i = rhs.meshes.begin();
    while (lhs.meshes.end() != lhs_i)
      swap(*lhs_i++, *rhs_i++);
		//swap(lhs.meshes, rhs.meshes);
    
		swap(lhs.meshes_size, rhs.meshes_size);
		swap(lhs.model_file_checksum_, rhs.model_file_checksum_);
	}

	model() : meshes_size(0) {}
	model( model&& other ) { swap(*this, other); }
	model( checksum_type model_file_checksum ) 
		: meshes_size(0), model_file_checksum_(model_file_checksum) {}

	model& operator=( model rhs ) { swap(*this, rhs); return *this; }

	bool is_loaded() const { return 0 < meshes_size; }
	bool has_lights() const { return !lights.empty(); }
	checksum_type model_file_checksum() const { return model_file_checksum_; }

	void push_back( mesh&& mesh ) { meshes[meshes_size++] = std::move(mesh); }
	void render() 
	{
		std::for_each(meshes.cbegin(), meshes.cbegin() + meshes_size, 
			[] ( const mesh& mesh ){ mesh.render(); });
	}

	void clear()
	{ 
		std::generate(meshes.begin(), meshes.begin() + meshes_size, [](){ return mesh(); });
		meshes_size = 0;
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

	template<typename char_type>
	friend std::basic_istream<char_type>& operator>>( 
		std::basic_istream<char_type>& stream, 
		model& model )
	{
		model.clear();

		stream.read(reinterpret_cast<char_type*>(&model.model_file_checksum_), sizeof(checksum_type));
		stream.read(reinterpret_cast<char_type*>(&model.meshes_size), sizeof(int));
		
		std::for_each(model.meshes.begin(), model.meshes.begin() + model.meshes_size, 
			[&] ( mesh& mesh ) { stream >> mesh; });

		light_container::size_type lights_size;
		stream.read(reinterpret_cast<char_type*>(&lights_size), sizeof(light_container::size_type));
		if (0 < lights_size)
		{
			model.lights.resize(lights_size);
			stream.read(reinterpret_cast<char_type*>(model.lights.data()), sizeof(light) * lights_size);
		}

		return stream;
	}

protected:
	model( const model& other ); // Possible, but did not bother

private:
	std::array<mesh, BLACK_LABEL_RENDERER_STORAGE_MODEL_MESHES_MAX> meshes;
	int meshes_size;
	light_container lights;
	checksum_type model_file_checksum_;
};

} // namespace gpu
} // namespace storage
} // namespace renderer
} // namespace black_label



#endif
