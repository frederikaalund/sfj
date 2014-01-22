#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/renderer/storage/gpu/texture.hpp>

#include <cassert>

#include <GL/glew.h>



namespace black_label {
namespace renderer {
namespace storage {
namespace gpu {

////////////////////////////////////////////////////////////////////////////////
/// Texture Base
////////////////////////////////////////////////////////////////////////////////
namespace detail {

texture_base::~texture_base()
{
	if (valid()) glDeleteTextures(1, &id);
}



void texture_base::bind( target_type target ) const
{
	glBindTexture(target, id);
}

void texture_base::set_parameters( target_type target, filter filter, wrap wrap ) const
{
	int mag_filter, min_filter, wrap_s, wrap_t;
	switch (filter)
	{
	case NEAREST:
		mag_filter = min_filter = GL_NEAREST;
		break;
	case LINEAR:
		mag_filter = min_filter = GL_LINEAR;
		break;
	case MIPMAP:
		mag_filter = GL_LINEAR;
		min_filter = GL_LINEAR_MIPMAP_LINEAR;
		break;
	default:
		assert(false);
		break;
	}

	switch (wrap)
	{
	case CLAMP_TO_EDGE:
		wrap_s = wrap_t = GL_CLAMP_TO_EDGE;
		break;
	case MIRRORED_REPEAT:
		wrap_s = wrap_t = GL_MIRRORED_REPEAT;
		break;
	case REPEAT:
		wrap_s = wrap_t = GL_REPEAT;
		break;
	default:
		assert(false);
		break;
	}

	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, mag_filter);
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, min_filter);
	glTexParameteri(target, GL_TEXTURE_WRAP_S, wrap_s);
	glTexParameteri(target, GL_TEXTURE_WRAP_T, wrap_t);
}

void texture_base::use( target_type target, const core_program& program, const char* name, int& texture_unit ) const
{
	glActiveTexture(GL_TEXTURE0 + texture_unit);
	program.set_uniform(name, texture_unit++);
	bind(target);
}



void texture_base::update( 
	target_type target,
	int width, 
	int height, 
	const std::uint8_t* rgba_data, 
	int mipmap_levels ) const
{
	if (0 >= mipmap_levels)
	{
		glTexImage2D(target, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba_data);
		return;
	}

	glTexStorage2D(target, mipmap_levels, GL_RGBA8, width, height);
	glTexSubImage2D(target, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgba_data);
	glGenerateMipmap(target);
}

void texture_base::update( 
	target_type target,
	int width, 
	int height, 
	const float* rgba_data, 
	int mipmap_levels ) const
{
	if (0 >= mipmap_levels)
	{
		glTexImage2D(target, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, rgba_data);
		return;
	}

	glTexStorage2D(target, mipmap_levels, GL_RGBA32F, width, height);
	glTexSubImage2D(target, 0, 0, 0, width, height, GL_RGBA, GL_FLOAT, rgba_data);
	glGenerateMipmap(target);
}

void texture_base::update( 
	target_type target,
	int width, 
	int height, 
	const glm::vec3* data, 
	int mipmap_levels ) const
{
	if (0 >= mipmap_levels)
	{
		glTexImage2D(target, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, reinterpret_cast<const float*>(data));
		return;
	}

	glTexStorage2D(target, mipmap_levels, GL_RGB32F, width, height);
	glTexSubImage2D(target, 0, 0, 0, width, height, GL_RGB, GL_FLOAT, reinterpret_cast<const float*>(data));
	glGenerateMipmap(target);
}

void texture_base::update( 
	target_type target,
	int width, 
	int height, 
	const depth_float* data, 
	int mipmap_levels ) const
{
	if (0 >= mipmap_levels)
	{
		glTexImage2D(target, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, data);
		return;
	}

	glTexStorage2D(target, mipmap_levels, GL_DEPTH_COMPONENT32F, width, height);
	glTexSubImage2D(target, 0, 0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, data);
	glGenerateMipmap(target);
}

void texture_base::update( 
	target_type target,
	int width, 
	int height, 
	const srgb_a* data, 
	int mipmap_levels ) const
{
	if (0 >= mipmap_levels)
	{
		glTexImage2D(target, 0, GL_SRGB8_ALPHA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		return;
	}

    if (GLEW_ARB_texture_storage)
    {
        glTexStorage2D(target, mipmap_levels, GL_SRGB8_ALPHA8, width, height);
        glTexSubImage2D(target, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
    }
    else
        glTexImage2D(target, 0, GL_SRGB8_ALPHA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        
    glGenerateMipmap(target);
}

void texture_base::update( 
	target_type target,
	int width, 
	int height, 
	int depth, 
	const float* data, 
	int mipmap_levels ) const
{
    if (0 >= mipmap_levels)
	{
		glTexImage3D(target, 0, GL_RGB, width, height, depth, 0, GL_RGB, GL_FLOAT, data);
		return;
	}

	glTexStorage3D(target, mipmap_levels, GL_RGB, width, height, depth);
	glTexSubImage3D(target, 0, 0, 0, 0, width, height, depth, GL_RGB, GL_FLOAT, data);
	glGenerateMipmap(target);
}



void texture_base::generate()
{
	glGenTextures(1, &id);
}

} // namespace detail



////////////////////////////////////////////////////////////////////////////////
/// Targets
////////////////////////////////////////////////////////////////////////////////
namespace target {
detail::texture_base::target_type tex2d() { return GL_TEXTURE_2D; }
detail::texture_base::target_type tex3d() { return GL_TEXTURE_3D; }
} // namespace target



////////////////////////////////////////////////////////////////////////////////
/// Texture Buffer
////////////////////////////////////////////////////////////////////////////////
namespace detail {

void texture_buffer_base::associate_as_int32s() const
{
	glTexBuffer(GL_TEXTURE_BUFFER, GL_R32I, buffer);
}
void texture_buffer_base::associate_as_ivec2s() const
{
	static_assert(4 == sizeof(glm::ivec2::value_type), "ivec2 does not use 4 byte integers!");
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32I, buffer);
}
void texture_buffer_base::associate_as_floats() const
{
	glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, buffer);
}
void texture_buffer_base::associate_as_vec2s() const
{
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32F, buffer);
}
void texture_buffer_base::associate_as_vec4s() const
{
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, buffer);
}

} // namespace detail

} // namespace gpu
} // namespace storage
} // namespace renderer
} // namespace black_label
