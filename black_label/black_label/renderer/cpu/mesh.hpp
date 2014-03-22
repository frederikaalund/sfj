#ifndef BLACK_LABEL_RENDERER_CPU_MESH_HPP
#define BLACK_LABEL_RENDERER_CPU_MESH_HPP

#include <black_label/renderer/cpu/texture.hpp>
#include <black_label/renderer/material.hpp>
#include <black_label/renderer/types_and_constants.hpp>
#include <black_label/utility/serialization/vector.hpp>

#include <utility>
#include <vector>

#include <boost/crc.hpp>
#include <boost/serialization/access.hpp>



namespace black_label {
namespace renderer {
namespace cpu {

class mesh
{
public:
	using vector_container = std::vector<float>;
	using index_container = std::vector<unsigned int>;

	friend class boost::serialization::access;



	friend void swap( mesh& lhs, mesh& rhs )
	{
		using std::swap;
		swap(lhs.vertices, rhs.vertices);
		swap(lhs.normals, rhs.normals);
		swap(lhs.texture_coordinates, rhs.texture_coordinates);
		swap(lhs.indices, rhs.indices);
		swap(lhs.render_mode, rhs.render_mode);
		swap(lhs.material, rhs.material);
	}

	mesh() {}
	mesh( 		
		material material,
		render_mode render_mode,
		vector_container vertices,
		vector_container normals = vector_container{},
		vector_container texture_coordinates = vector_container{},
		index_container indices = index_container{} )
		: vertices{std::move(vertices)}
		, normals{std::move(normals)}
		, texture_coordinates{std::move(texture_coordinates)}
		, indices{std::move(indices)}
		, render_mode{std::move(render_mode)}
		, material{std::move(material)}
	{}
	
	mesh( const mesh& ) = delete;
	mesh( mesh&& other ) : mesh{} { swap(*this, other); }
	mesh& operator=( mesh rhs ) { swap(*this, rhs); return *this; }



	template<typename archive_type>
	void serialize( archive_type& archive, unsigned int version )
	{
		archive & vertices & normals & texture_coordinates & indices
			& render_mode & material;
	}



	vector_container vertices, normals, texture_coordinates;
	index_container indices;
	render_mode render_mode;
	material material;
};

} // namespace cpu
} // namespace renderer
} // namespace black_label



#endif
