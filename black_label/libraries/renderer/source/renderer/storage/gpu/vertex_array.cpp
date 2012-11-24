#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/renderer/storage/gpu/vertex_array.hpp>

#include <GL/glew.h>



namespace black_label {
namespace renderer {
namespace storage {
namespace gpu {

void vertex_array::generate()
{
	glGenVertexArrays(1, &id);
}

vertex_array::~vertex_array()
{
	if (valid()) glDeleteVertexArrays(1, &id);
}

void vertex_array::bind() const
{
	glBindVertexArray(id);
}

void vertex_array::add_attribute( index_type& index, int size, const void* offset ) const
{
	glVertexAttribPointer(index, size, GL_FLOAT, GL_FALSE, 0, offset); 
	glEnableVertexAttribArray(index++);
}

} // namespace gpu
} // namespace storage
} // namespace renderer
} // namespace black_label