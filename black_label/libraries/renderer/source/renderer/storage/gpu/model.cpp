#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/renderer/storage/gpu/model.hpp>
#include <black_label/utility/log_severity_level.hpp>
#include <black_label/utility/scoped_stream_suppression.hpp>

#include <cassert>
#include <string>

#include <boost/log/common.hpp>
#include <boost/log/sources/severity_logger.hpp>

#include <GL/glew.h>

#include <SFML/Graphics/Image.hpp>

using namespace std;
using namespace sf;



namespace black_label {
namespace renderer {
namespace storage {
namespace gpu {

mesh::mesh( const cpu::mesh& cpu_mesh )
	: render_mode(cpu_mesh.render_mode)
	, material(cpu_mesh.material)
{
	const float* normals_begin = (!cpu_mesh.normals.empty()) ? cpu_mesh.normals.data() : nullptr;
	const float* texture_coordinates_begin = (!cpu_mesh.texture_coordinates.empty()) ? cpu_mesh.texture_coordinates.data() : nullptr;
	const unsigned int* indices_begin = (!cpu_mesh.indices.empty()) ? cpu_mesh.indices.data() : nullptr;
	const unsigned int* indices_end = (indices_begin) ? &cpu_mesh.indices[cpu_mesh.indices.capacity()] : nullptr;

	load(
		cpu_mesh.vertices.data(), 
		&cpu_mesh.vertices[cpu_mesh.vertices.capacity()],
		normals_begin,
		texture_coordinates_begin,
		indices_begin,
		indices_end);
}

mesh::mesh( 
	const black_label::renderer::material& material,
	render_mode_type render_mode, 
	const float* vertices_begin,
	const float* vertices_end,
	const float* normals_begin,
	const float* texture_coordinates_begin,
	const unsigned int* indices_begin,
	const unsigned int* indices_end )
	: render_mode(render_mode)
	, material(material)
{
	load(
		vertices_begin, 
		vertices_end, 
		normals_begin, 
		texture_coordinates_begin,
		indices_begin, 
		indices_end);
}



void mesh::render( const core_program& program, int& texture_unit ) const
{
	if (material)
	{
		if (!material.specular_texture.empty())
		{
			program.set_uniform("specular_exponent", 1.0f);
			specular_texture.use(program, "specular_texture", texture_unit);
		}
		else
			program.set_uniform("specular_exponent", 0.0f);

		if (!material.diffuse_texture.empty())
			diffuse_texture.use(program, "diffuse_texture", texture_unit);
	}

	render_without_material();
}

void mesh::render_without_material() const
{
	vertex_array.bind();

	if (has_indices())
		glDrawElements(render_mode, draw_count, GL_UNSIGNED_INT, nullptr);
	else
		glDrawArrays(render_mode, 0, draw_count);
}

void mesh::load(
	const float* vertices_begin,
	const float* vertices_end,
	const float* normals_begin,
	const float* texture_coordinates_begin,
	const unsigned int* indices_begin,
	const unsigned int* indices_end )
{
	////////////////////////////////////////////////////////////////////////////////
	/// Vertex Array Object
	////////////////////////////////////////////////////////////////////////////////
	vertex_array = gpu::vertex_array(generate);
	vertex_array.bind();



	////////////////////////////////////////////////////////////////////////////////
	/// Vertices
	////////////////////////////////////////////////////////////////////////////////
	draw_count = vertices_end - vertices_begin;
	GLsizeiptr vertex_size = draw_count * sizeof(float);
	GLsizeiptr normal_size = (normals_begin) ? vertex_size : 0;
	GLsizeiptr texture_coordinate_size = (texture_coordinates_begin) ? vertex_size * 2 / 3 : 0;
	auto total_size = vertex_size + normal_size + texture_coordinate_size;

	
	
	vertex_buffer = vertex_buffer_type(total_size);
	
	GLintptr offset = 0;
	gpu::vertex_array::index_type index = 0;
	vertex_buffer.update(offset, vertex_size, vertices_begin);
	vertex_array.add_attribute(index, 3, nullptr);
	offset += vertex_size;
	if (normals_begin)
	{
		vertex_buffer.update(offset, normal_size, normals_begin);
		vertex_array.add_attribute(index, 3, reinterpret_cast<const void*>(offset));
		offset += normal_size;
	}
	if (texture_coordinates_begin)
	{
		vertex_buffer.update(offset, texture_coordinate_size, texture_coordinates_begin);
		vertex_array.add_attribute(index, 2, reinterpret_cast<const void*>(offset));
		offset += texture_coordinate_size;
	}	



	////////////////////////////////////////////////////////////////////////////////
	/// Indices
	////////////////////////////////////////////////////////////////////////////////
	if (indices_begin != indices_end)
	{
		draw_count = indices_end - indices_begin;
		index_buffer = index_buffer_type(draw_count * sizeof(unsigned int), indices_begin);
	}
	else
		draw_count /= 3;



	////////////////////////////////////////////////////////////////////////////////
	/// Texture
	////////////////////////////////////////////////////////////////////////////////
	if (!material.diffuse_texture.empty())
	{
		using namespace black_label::utility;
		boost::log::sources::severity_logger<severity_level> log;

		Image image;
		{
			scoped_stream_suppression suppress(stdout);
			if (!image.loadFromFile(material.diffuse_texture))
			{
				BOOST_LOG_SEV(log, warning) << "Missing texture \"" << material.diffuse_texture << "\"";
				return;
			}
		}

		diffuse_texture = texture_2d(MIPMAP, REPEAT, image.getSize().x, image.getSize().y, reinterpret_cast<const texture_2d::srgb_a*>(image.getPixelsPtr()));
		BOOST_LOG_SEV(log, info) << "Imported texture \"" << material.diffuse_texture << "\"";
	}

	if (!material.specular_texture.empty())
	{
		using namespace black_label::utility;
		boost::log::sources::severity_logger<severity_level> log;

		Image image;
		{
			scoped_stream_suppression suppress(stdout);
			if (!image.loadFromFile(material.specular_texture))
			{
				BOOST_LOG_SEV(log, warning) << "Missing texture \"" << material.specular_texture << "\"";
				return;
			}
		}

		specular_texture = texture_2d(MIPMAP, REPEAT, image.getSize().x, image.getSize().y, reinterpret_cast<const texture_2d::srgb_a*>(image.getPixelsPtr()));
		BOOST_LOG_SEV(log, info) << "Imported texture \"" << material.specular_texture << "\"";
	}
}

} // namespace gpu
} // namespace storage
} // namespace renderer
} // namespace black_label