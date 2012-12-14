#ifndef BLACK_LABEL_RENDERER_STORAGE_CPU_MODEL_HPP
#define BLACK_LABEL_RENDERER_STORAGE_CPU_MODEL_HPP

#include <black_label/file_buffer.hpp>
#include <black_label/renderer/light.hpp>
#include <black_label/renderer/material.hpp>
#include <black_label/shared_library/utility.hpp>
#include <black_label/utility/algorithm.hpp>
#include <black_label/utility/serialization/darray.hpp>
#include <black_label/utility/serialization/vector.hpp>

#include <algorithm>
#include <bitset>
#include <memory>

#include <boost/crc.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/array.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>



namespace black_label {
namespace renderer {

class renderer;

namespace storage {
namespace cpu {

////////////////////////////////////////////////////////////////////////////////
/// Mesh
////////////////////////////////////////////////////////////////////////////////
typedef unsigned int render_mode_type;

class mesh
{
public:
	typedef container::darray<float> vector_container;
	typedef container::darray<unsigned int> index_container;

	friend class boost::serialization::access;



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



	template<typename archive_type>
	void serialize( archive_type& archive, unsigned int version )
	{
		archive & vertices & normals & texture_coordinates & indices
			& render_mode & material;
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
typedef boost::crc_32_type::value_type checksum_type;

class model
{
public:
	typedef std::vector<light> light_container;

	friend class renderer;
	friend class boost::serialization::access;



	model() {}
	model( checksum_type model_file_checksum ) 
		: model_file_checksum_(model_file_checksum) {}

	checksum_type model_file_checksum() const { return model_file_checksum_; }

	void add_light( const light& light ) { lights.push_back(light); }

	void push_back( mesh&& mesh ) { meshes.push_back(std::move(mesh)); }



	template<typename archive_type>
	void serialize( archive_type& archive, unsigned int version )
	{
		archive.save_binary(&model_file_checksum_, sizeof(checksum_type));
		archive & meshes & lights;
	}



protected:
private:
	std::vector<mesh> meshes;
	light_container lights;
	checksum_type model_file_checksum_;
};

} // namespace cpu
} // namespace storage
} // namespace renderer
} // namespace black_label



#endif
