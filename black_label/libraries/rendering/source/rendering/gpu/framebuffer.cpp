#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/rendering/gpu/framebuffer.hpp>

#include <GL/glew.h>



namespace black_label {
namespace rendering {
namespace gpu {



////////////////////////////////////////////////////////////////////////////////
/// Basic Frameuffer
////////////////////////////////////////////////////////////////////////////////
basic_framebuffer::~basic_framebuffer()
{ if (valid()) glDeleteFramebuffers(1, &id); }

void basic_framebuffer::generate()
{ glGenFramebuffers(1, &id); }

void basic_framebuffer::bind() const
{ glBindFramebuffer(GL_FRAMEBUFFER, id); }

void basic_framebuffer::unbind()
{ glBindFramebuffer(GL_FRAMEBUFFER, 0); }

void basic_framebuffer::attach_texture_2d( attachment_type attachment, basic_texture::id_type texture ) const 
{ glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture, 0); }

void basic_framebuffer::detach( attachment_type attachment )
{ glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, 0, 0, 0); }

void basic_framebuffer::set_draw_buffers( int size, const attachment_type* buffers ) const
{ glDrawBuffers(size, buffers); }

bool basic_framebuffer::is_complete() const
{
	auto framebuffer_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	return GL_FRAMEBUFFER_COMPLETE == framebuffer_status;
}



////////////////////////////////////////////////////////////////////////////////
/// Frameuffer
////////////////////////////////////////////////////////////////////////////////
framebuffer::attachment_type framebuffer::attach( const texture& texture, unsigned int& attachment_index ) const
{
	GLenum attachment = (format::is_depth_format(texture.format))
		? GL_DEPTH_ATTACHMENT
		: GL_COLOR_ATTACHMENT0 + attachment_index++;

	assert(target::texture_2d == texture.target);
	attach_texture_2d(attachment, texture);

	return attachment;
}

} // namespace gpu
} // namespace rendering
} // namespace black_label
