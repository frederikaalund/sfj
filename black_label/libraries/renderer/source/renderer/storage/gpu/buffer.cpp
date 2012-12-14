#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/renderer/storage/gpu/buffer.hpp>

#include <GL/glew.h>



namespace black_label {
namespace renderer {
namespace storage {
namespace gpu {



namespace detail {

////////////////////////////////////////////////////////////////////////////////
/// Buffer Base
////////////////////////////////////////////////////////////////////////////////
buffer_base::~buffer_base()
{
	if (valid()) glDeleteBuffers(1, &id);
}

void buffer_base::generate()
{
	glGenBuffers(1, &id);
}

void buffer_base::bind( target_type target ) const
{
	glBindBuffer(target, id);
}

void buffer_base::update( target_type target, usage_type usage, size_type size, const void* data ) const
{
	glBufferData(target, size, data, usage);
}

void buffer_base::update( target_type target, offset_type offset, size_type size, const void* data ) const
{
	glBufferSubData(target, offset, size, data);
}

} // namespace detail



////////////////////////////////////////////////////////////////////////////////
/// Targets and Usages
////////////////////////////////////////////////////////////////////////////////
namespace target {
detail::buffer_base::target_type texture::get() { return GL_TEXTURE_BUFFER; }
detail::buffer_base::target_type array::get() { return GL_ARRAY_BUFFER; }
detail::buffer_base::target_type element_array::get() { return GL_ELEMENT_ARRAY_BUFFER; }
} // namespace target

namespace usage {
detail::buffer_base::usage_type stream_draw::get() { return GL_STREAM_DRAW; }
detail::buffer_base::usage_type static_draw::get() { return GL_STATIC_DRAW; }
} // namespace usage

} // namespace gpu
} // namespace storage
} // namespace renderer
} // namespace black_label