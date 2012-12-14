#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/renderer/storage/gpu/model.hpp>

#include <cassert>
#include <string>

#include <GL/glew.h>

using namespace std;



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
	const unsigned int* indices_begin = (!cpu_mesh.indices.empty()) ? cpu_mesh.indices.data() : nullptr;
	const unsigned int* indices_end = (indices_begin) ? &cpu_mesh.indices[cpu_mesh.indices.capacity()] : nullptr;

	load(
		cpu_mesh.vertices.data(), 
		&cpu_mesh.vertices[cpu_mesh.vertices.capacity()],
		normals_begin,
		indices_begin,
		indices_end);
}

mesh::mesh( 
	const black_label::renderer::material& material,
	render_mode_type render_mode, 
	const float* vertices_begin,
	const float* vertices_end,
	const float* normals_begin,
	const unsigned int* indices_begin,
	const unsigned int* indices_end )
	: render_mode(render_mode)
	, material(material)
{
	load(
		vertices_begin, 
		vertices_end, 
		normals_begin, 
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
	draw_count = vertices_end-vertices_begin;
	GLsizeiptr attribute_size = draw_count*sizeof(float);
	GLsizeiptr total_size = attribute_size;
	if (nullptr == normals_begin)
		normal_size = 0;
	else
	{
		normal_size = attribute_size;
		total_size *= 2;
	}

	glGenBuffers(1, &vertex_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_vbo);
	glBufferData(
		GL_ARRAY_BUFFER, 
		total_size,
		nullptr,
		GL_STATIC_DRAW);
	
	glBufferSubData(GL_ARRAY_BUFFER, 0, attribute_size, vertices_begin);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr); 
	glEnableVertexAttribArray(0);   
	if (normal_size)
	{
		glBufferSubData(GL_ARRAY_BUFFER, attribute_size, normal_size, normals_begin);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid*>(normal_size));
		glEnableVertexAttribArray(1);
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
		draw_count*sizeof(unsigned int),
		indices_begin,
		GL_STATIC_DRAW);
}

} // namespace gpu
} // namespace storage
} // namespace renderer
} // namespace black_label