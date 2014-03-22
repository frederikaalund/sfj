#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/renderer/gpu/mesh.hpp>

#include <GL/glew.h>



namespace black_label {
namespace renderer {

render_mode::render_mode() : internal{GL_TRIANGLES} {}

namespace gpu {

void mesh::render( const core_program& program, unsigned int texture_unit ) const
{
	if (diffuse && diffuse->valid())
		diffuse->use(program, "diffuse_texture", texture_unit);
	else
		program.set_uniform("diffuse_texture", 0);

	if (specular && specular->valid())
	{
		specular->use(program, "specular_texture", texture_unit);
		program.set_uniform("specular_exponent", 1.0f);
	}
	else
		program.set_uniform("specular_exponent", 0.0f);

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
	draw_count = static_cast<int>(vertices_end - vertices_begin);
	GLsizeiptr vertex_size = draw_count * sizeof(float);
	GLsizeiptr normal_size = (normals_begin) ? vertex_size : 0;
	GLsizeiptr texture_coordinate_size = (texture_coordinates_begin) ? vertex_size * 2 / 3 : 0;
	auto total_size = vertex_size + normal_size + texture_coordinate_size;

	
	vertex_buffer = buffer{target::array, usage::static_draw, total_size};

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
		draw_count = static_cast<int>(indices_end - indices_begin);
		index_buffer = buffer{target::element_array, usage::static_draw, draw_count * static_cast<GLsizeiptr>(sizeof(unsigned int)), indices_begin};
	}
	else
		draw_count /= 3;
}

} // namespace gpu
} // namespace renderer
} // namespace black_label