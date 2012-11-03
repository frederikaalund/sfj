#ifndef BLACK_LABEL_RENDERER_STORAGE_CPU_MODEL_HPP
#define BLACK_LABEL_RENDERER_STORAGE_CPU_MODEL_HPP

#include <black_label/file_buffer.hpp>
#include <black_label/renderer/light.hpp>
#include <black_label/renderer/material.hpp>
#include <black_label/shared_library/utility.hpp>
#include <black_label/utility/algorithm.hpp>

#include <algorithm>
#include <array>
#include <bitset>
#include <memory>
#include <istream>
#include <ostream>
#include <vector>

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
namespace cpu
{

////////////////////////////////////////////////////////////////////////////////
/// Mesh
////////////////////////////////////////////////////////////////////////////////
typedef unsigned int render_mode_type;

class mesh
{
public:
	typedef container::darray<float> vector_container;
	typedef container::darray<unsigned int> index_container;

	mesh() {}
	mesh(
		const material& material,
		render_mode_type render_mode,
		vector_container&& vertices,
		vector_container&& normals,
		vector_container&& texture_coordinates,
		index_container&& indices )
		: vertices(std::move(vertices))
		, normals(std::move(normals))
		, texture_coordinates(std::move(texture_coordinates))
		, indices(std::move(indices))
		, render_mode(render_mode)
		, material(material)
	{}
	mesh(
		const material& material,
		render_mode_type render_mode,
		float* vertices_begin,
		float* vertices_end,
		float* normals_begin = nullptr,
		float* texture_coordinates_begin = nullptr,
		unsigned int* indices_begin = nullptr,
		unsigned int* indices_end = nullptr )
		: vertices(vertices_begin, vertices_end)
		, normals((normals_begin) ? vector_container(normals_begin, normals_begin + (vertices_end - vertices_begin)) : vector_container())
		, texture_coordinates((texture_coordinates_begin) ? vector_container(texture_coordinates_begin, texture_coordinates_begin + (vertices_end - vertices_begin) * 2 / 3) : vector_container())
		, indices((indices_begin) ? index_container(indices_begin, indices_end) : index_container())
		, render_mode(render_mode)
		, material(material)
	{}

	template<typename char_type>
	friend std::basic_ostream<char_type>& operator<<( 
		std::basic_ostream<char_type>& stream, 
		const mesh& mesh )
	{
		vector_container::size_type 
			vertices_capacity = mesh.vertices.capacity(),
			normals_capacity = mesh.normals.capacity(),
			texture_coordinates_capacity = mesh.texture_coordinates.capacity();
		index_container::size_type
			indices_capacity = mesh.indices.capacity();

		stream.write(reinterpret_cast<const char_type*>(&mesh.render_mode), sizeof(render_mode_type));
		stream.write(reinterpret_cast<const char_type*>(&vertices_capacity), sizeof(vector_container::size_type));
		stream.write(reinterpret_cast<const char_type*>(&normals_capacity), sizeof(vector_container::size_type));
		stream.write(reinterpret_cast<const char_type*>(&texture_coordinates_capacity), sizeof(vector_container::size_type));
		stream.write(reinterpret_cast<const char_type*>(&indices_capacity), sizeof(index_container::size_type));

		stream.write(reinterpret_cast<const char_type*>(mesh.vertices.data()), mesh.vertices.capacity() * sizeof(vector_container::value_type));
		if (!mesh.normals.empty())
			stream.write(reinterpret_cast<const char_type*>(mesh.normals.data()), mesh.normals.capacity() * sizeof(vector_container::value_type));
		if (!mesh.texture_coordinates.empty())
			stream.write(reinterpret_cast<const char_type*>(mesh.texture_coordinates.data()), mesh.texture_coordinates.capacity() * sizeof(vector_container::value_type));
		if (!mesh.indices.empty())
			stream.write(reinterpret_cast<const char_type*>(mesh.indices.data()), mesh.indices.capacity() * sizeof(index_container::value_type));

		stream << mesh.material;

		return stream;
	}



	vector_container vertices, normals, texture_coordinates;
	index_container indices;
	render_mode_type render_mode;
	material material;

protected:
private:
};



////////////////////////////////////////////////////////////////////////////////
/// Model
////////////////////////////////////////////////////////////////////////////////
#define BLACK_LABEL_RENDERER_STORAGE_MODEL_MESHES_MAX 64
typedef boost::crc_32_type::value_type checksum_type;

class model
{
public:
	typedef std::vector<light> light_container;

	friend class renderer;

	model() : meshes_size(0) {}
	model( checksum_type model_file_checksum ) 
		: meshes_size(0), model_file_checksum_(model_file_checksum) {}

	checksum_type model_file_checksum() const { return model_file_checksum_; }

	void add_light( const light& light ) { lights.push_back(light); }

	void push_back( mesh&& mesh ) { meshes[meshes_size++] = std::move(mesh); }

	template<typename char_type>
	friend std::basic_ostream<char_type>& operator<<( 
		std::basic_ostream<char_type>& stream, 
		const model& model )
	{
		stream.write(reinterpret_cast<const char_type*>(&model.model_file_checksum_), sizeof(checksum_type));
		stream.write(reinterpret_cast<const char_type*>(&model.meshes_size), sizeof(int));

		std::for_each(model.meshes.cbegin(), model.meshes.cbegin() + model.meshes_size, 
			[&] ( const mesh& mesh ) { stream << mesh; });

		auto lights_size = model.lights.size();
		stream.write(reinterpret_cast<const char_type*>(&lights_size), sizeof(light_container::size_type));
		if (0 < lights_size)
			stream.write(reinterpret_cast<const char_type*>(model.lights.data()), sizeof(light) * lights_size);

		return stream;
	}

protected:
private:
	std::array<mesh, BLACK_LABEL_RENDERER_STORAGE_MODEL_MESHES_MAX> meshes;
	int meshes_size;
	light_container lights;
	checksum_type model_file_checksum_;
};

} // namespace cpu
} // namespace storage
} // namespace renderer
} // namespace black_label



#endif
