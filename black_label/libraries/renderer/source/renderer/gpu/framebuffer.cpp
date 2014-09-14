#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/renderer/gpu/framebuffer.hpp>

#include <GL/glew.h>



namespace black_label {
namespace renderer {
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


    
} // namespace gpu
} // namespace renderer
} // namespace black_label