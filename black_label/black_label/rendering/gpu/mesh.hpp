#ifndef BLACK_LABEL_RENDERING_GPU_MESH_HPP
#define BLACK_LABEL_RENDERING_GPU_MESH_HPP

#include <memory>

#include <black_label/rendering/material.hpp>
#include <black_label/rendering/program.hpp>
#include <black_label/rendering/cpu/model.hpp>
#include <black_label/rendering/gpu/argument/mesh.hpp>
#include <black_label/rendering/gpu/texture.hpp>
#include <black_label/rendering/gpu/vertex_array.hpp>
#include <black_label/utility/threading_building_blocks/path.hpp>

#include <boost/crc.hpp>

#include <tbb/concurrent_hash_map.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>



namespace black_label {
namespace rendering {
namespace gpu {

using texture_map = concurrent_resource_map<texture>;

class mesh
{
public:
////////////////////////////////////////////////////////////////////////////////
/// Configuration
////////////////////////////////////////////////////////////////////////////////
	class configuration
	{
	public:
		// Implicitly constructible from vertices
		configuration( argument::vertices vertices ) : vertices(vertices) {}
		// Implicitly constructible from cpu_mesh
		configuration( const cpu::mesh& cpu_mesh ) 
			: vertices(cpu_mesh.vertices.data(), cpu_mesh.vertices.data() + cpu_mesh.vertices.size())
			, normals(cpu_mesh.normals.data(), cpu_mesh.normals.data() + cpu_mesh.normals.size())
			, texture_coordinates(cpu_mesh.texture_coordinates.data(), cpu_mesh.texture_coordinates.data() + cpu_mesh.texture_coordinates.size())
			, indices(cpu_mesh.indices.data(), cpu_mesh.indices.data() + cpu_mesh.indices.size())
			, draw_mode{cpu_mesh.draw_mode}
			, material{cpu_mesh.material}
		{}

		configuration& set( argument::normals value ) 
		{ this->normals = value; return *this; }
		configuration& set( argument::texture_coordinates value ) 
		{ this->texture_coordinates = value; return *this; }
		configuration& set( argument::indices value ) 
		{ this->indices = value; return *this; }
		configuration& set( draw_mode value ) 
		{ this->draw_mode = value; return *this; }
		configuration& set( material value ) 
		{ this->material = value; return *this; }

		argument::vertices vertices;
		argument::normals normals;
		argument::texture_coordinates texture_coordinates;
		argument::indices indices;
		draw_mode draw_mode;
		material material;
	};



////////////////////////////////////////////////////////////////////////////////
/// Mesh
////////////////////////////////////////////////////////////////////////////////
	friend void swap( mesh& lhs, mesh& rhs )
	{
		using std::swap;
		swap(lhs.draw_count, rhs.draw_count);
		swap(lhs.vertex_buffer, rhs.vertex_buffer);
		swap(lhs.index_buffer, rhs.index_buffer);
		swap(lhs.vertex_array, rhs.vertex_array);
		swap(lhs.draw_mode, rhs.draw_mode);
		swap(lhs.material, rhs.material);
		swap(lhs.diffuse, rhs.diffuse);
		swap(lhs.specular, rhs.specular);
	}

	mesh() {}
	mesh(
		const material& material,
		draw_mode draw_mode ) 
		: draw_mode{draw_mode}, material{material}
	{}
	mesh( configuration configuration )
		: mesh{configuration.material, configuration.draw_mode}
	{ load(configuration); }
	mesh( configuration configuration, texture_map& textures )
		: mesh{configuration}
	{ 
		texture_map::const_accessor texture;
		if (textures.find(texture, material.diffuse_texture))
			diffuse = texture->second.lock();
		if (textures.find(texture, material.specular_texture))
			specular = texture->second.lock();
	}
	mesh( const mesh& ) = delete; // Possible, but do you really want to?
	mesh( mesh&& other ) : mesh{} { swap(*this, other); }
	mesh& operator=( mesh rhs ) { swap(*this, rhs); return *this; }

	void load(
		const float* vertices_begin,
		const float* vertices_end,
		const float* normals_begin = nullptr,
		const float* texture_coordinates_begin = nullptr,
		const unsigned int* indices_begin = nullptr,
		const unsigned int* indices_end = nullptr );
	void load( configuration configuration )
	{
		using namespace std;
		load(
			cbegin(configuration.vertices),
			cend(configuration.vertices),
			cbegin(configuration.normals),
			cbegin(configuration.texture_coordinates),
			cbegin(configuration.indices),
			cend(configuration.indices));
	}

	bool is_loaded() const { return vertex_buffer.valid(); }
	bool has_indices() const { return index_buffer.valid(); }

	void render( const core_program& program, unsigned int texture_unit ) const;
	void render() const;



	int draw_count;
	buffer vertex_buffer, index_buffer;
	vertex_array vertex_array;
	draw_mode draw_mode;
	material material;
	std::shared_ptr<texture> diffuse, specular;
};



template<typename T>
inline mesh::configuration operator+( mesh::configuration configuration, T value )
{ return configuration.set(value); }

} // namespace gpu
} // namespace rendering
} // namespace black_label



#endif
