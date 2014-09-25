#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/rendering/gpu/buffer.hpp>

#include <GL/glew.h>



namespace black_label {
namespace rendering {
namespace gpu {



////////////////////////////////////////////////////////////////////////////////
/// Targets and Usages
////////////////////////////////////////////////////////////////////////////////
namespace target {
	const type 
		texture = GL_TEXTURE_BUFFER,
		array = GL_ARRAY_BUFFER,
		element_array = GL_ELEMENT_ARRAY_BUFFER,
		uniform_buffer = GL_UNIFORM_BUFFER,
		shader_storage = GL_SHADER_STORAGE_BUFFER,
		atomic_counter = GL_ATOMIC_COUNTER_BUFFER;
} // namespace target

namespace usage {
	const type
		stream_draw = GL_STREAM_DRAW,
		static_draw = GL_STATIC_DRAW,
		dynamic_draw = GL_DYNAMIC_DRAW,
		dynamic_copy = GL_DYNAMIC_COPY;
} // namespace usage



////////////////////////////////////////////////////////////////////////////////
/// Basic Buffer
////////////////////////////////////////////////////////////////////////////////
basic_buffer::~basic_buffer()
{ if (valid()) glDeleteBuffers(1, &id); }

void basic_buffer::generate()
{ glGenBuffers(1, &id); }

void basic_buffer::bind( target::type target ) const
{ glBindBuffer(target, id); }

void basic_buffer::unbind( target::type target )
{ glBindBuffer(target, 0); }

void basic_buffer::unbind()
{
	unbind(target::texture);
	unbind(target::array);
	unbind(target::element_array);
	unbind(target::uniform_buffer);
	unbind(target::shader_storage);
	unbind(target::atomic_counter);
}

void basic_buffer::bind( target::type target, index_type index ) const
{ glBindBufferBase(target, index, id); }

void basic_buffer::update( target::type target, usage::type usage, size_type size, const void* data ) const
{ glBufferData(target, size, data, usage); }

void basic_buffer::update( target::type target, offset_type offset, size_type size, const void* data ) const
{ glBufferSubData(target, offset, size, data); }


    
} // namespace gpu
} // namespace rendering
} // namespace black_label