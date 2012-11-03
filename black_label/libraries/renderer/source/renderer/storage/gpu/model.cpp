#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/renderer/storage/gpu/model.hpp>

#include <cassert>
#include <string>

#include <GL/glew.h>

#include <SFML/Graphics/Image.hpp>

using namespace std;
using namespace sf;



namespace black_label
{
namespace renderer
{
namespace storage
{
namespace gpu
{

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

mesh::~mesh()
{
	if (!is_loaded()) return;

	glDeleteBuffers(1, &vertex_vbo);
	if (has_indices()) glDeleteBuffers(1, &index_vbo);
	glDeleteVertexArrays(1, &vao);
}



void mesh::render( program::id_type program_id ) const
{
	glUniform4f(glGetUniformLocation(program_id, "color"),
		material.diffuse.r, 
		material.diffuse.g, 
		material.diffuse.b,
		material.alpha);
  

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, diffuse_texture);
	glUniform1i(glGetUniformLocation(program_id, "texture1"), 0);

	glBindVertexArray(vao);

	if (has_indices())
		glDrawElements(render_mode, draw_count, GL_UNSIGNED_INT, 0);
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
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);



	////////////////////////////////////////////////////////////////////////////////
	/// Vertices
	////////////////////////////////////////////////////////////////////////////////
	draw_count = vertices_end - vertices_begin;
	GLsizeiptr vertex_size = draw_count * sizeof(float);
	GLsizeiptr normal_size = (normals_begin) ? vertex_size : 0;
	GLsizeiptr texture_coordinate_size = (texture_coordinates_begin) ? vertex_size * 2 / 3 : 0;
	auto total_size = vertex_size + normal_size + texture_coordinate_size;

	glGenBuffers(1, &vertex_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_vbo);
	glBufferData(
		GL_ARRAY_BUFFER, 
		total_size,
		nullptr,
		GL_STATIC_DRAW);
	
	GLintptr offset = 0;
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertex_size, vertices_begin);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr); 
	glEnableVertexAttribArray(0);   
	offset += vertex_size;
	if (normals_begin)
	{
		glBufferSubData(GL_ARRAY_BUFFER, offset, normal_size, normals_begin);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid*>(offset));
		glEnableVertexAttribArray(1);
		offset += normal_size;
	}
	if (texture_coordinates_begin)
	{
		glBufferSubData(GL_ARRAY_BUFFER, offset, texture_coordinate_size, texture_coordinates_begin);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid*>(offset));
		glEnableVertexAttribArray(2);
		offset += texture_coordinate_size;
	}	



	////////////////////////////////////////////////////////////////////////////////
	/// Indices
	////////////////////////////////////////////////////////////////////////////////
	if (indices_begin == indices_end)
	{
		index_vbo = invalid_vbo;

		switch (render_mode)
		{
		// (2 indices)/(6 floats) = 1/3 indices/float
		case GL_LINES:
			{ draw_count /= 3; }
			break;
		// (4 indices)/(12 floats) = 1/3 indices/float
		case GL_QUADS:
			{ draw_count /= 3; }
			break;
		default:
			{ assert(false); }
		}

		return;
	}

	draw_count = indices_end-indices_begin;

	glGenBuffers(1, &index_vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_vbo);

	glBufferData(
		GL_ELEMENT_ARRAY_BUFFER, 
		draw_count * sizeof(unsigned int),
		indices_begin,
		GL_STATIC_DRAW);



	////////////////////////////////////////////////////////////////////////////////
	/// Texture
	////////////////////////////////////////////////////////////////////////////////
	if (!material.diffuse_texture.empty())
	{
		Image image;
		if (!image.loadFromFile(material.diffuse_texture))
 			return;

		image.flipVertically();

		glGenTextures(1, &diffuse_texture);
		glBindTexture(GL_TEXTURE_2D, diffuse_texture);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.getSize().x, image.getSize().y, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, image.getPixelsPtr());
	}

}

} // namespace gpu
} // namespace storage
} // namespace renderer
} // namespace black_label