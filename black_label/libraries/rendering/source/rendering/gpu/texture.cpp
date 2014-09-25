#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/rendering/gpu/texture.hpp>

#include <cassert>

#include <GL/glew.h>



namespace black_label {
namespace rendering {
namespace gpu {

namespace filter {
	const type 
		nearest{GL_NEAREST}, 
		linear{GL_LINEAR}, 
		mipmap{GL_LINEAR_MIPMAP_LINEAR};
} // namespace filter

namespace format {
	const type 
		r32i{GL_R32I},
		r32f{GL_R32F},
		rg32i{GL_RG32I},
		rgba8{GL_RGBA8}, 
		rgba16f{GL_RGBA16F}, 
		rgba32f{GL_RGBA32F}, 
		srgb{GL_SRGB8_ALPHA8},
		compressed_srgb{GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT},
		depth16{GL_DEPTH_COMPONENT16},
		depth24{GL_DEPTH_COMPONENT24},
		depth32f{GL_DEPTH_COMPONENT32F};
} // namespace format

namespace wrap {
	const type 
		clamp_to_edge{GL_CLAMP_TO_EDGE}, 
		repeat{GL_REPEAT},
		mirrored_repeat{GL_MIRRORED_REPEAT};
} // namespace wrap

namespace target {
	const type
		texture_2d{GL_TEXTURE_2D}, 
		texture_3d{GL_TEXTURE_3D},
		texture_buffer{GL_TEXTURE_BUFFER};
} // namespace target

namespace data_format {
	const type 
		rgb{GL_RGB}, 
		rgba{GL_RGBA}, 
		depth{GL_DEPTH_COMPONENT};
} // namespace data_format

namespace data_type {
	const type 
		uint8{GL_UNSIGNED_BYTE}, 
		float32{GL_FLOAT};
} // namespace data_type



////////////////////////////////////////////////////////////////////////////////
/// Basic Texture
////////////////////////////////////////////////////////////////////////////////

basic_texture::~basic_texture()
{ if (valid()) glDeleteTextures(1, &id); }



void basic_texture::bind( target::type target ) const
{ glBindTexture(target, id); }

void basic_texture::set_parameters( target::type target, filter::type filter, wrap::type wrap ) const
{
	filter::type mag_filter, min_filter;
	if (GL_LINEAR_MIPMAP_LINEAR == filter)
	{
		mag_filter = filter::linear;
		min_filter = filter;
	}
	else
		mag_filter = min_filter = filter;

	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, mag_filter);
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, min_filter);
	glTexParameteri(target, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(target, GL_TEXTURE_WRAP_T, wrap);
}

void basic_texture::use( target::type target, const core_program& program, const char* name, unsigned int& texture_unit ) const
{
	glActiveTexture(GL_TEXTURE0 + texture_unit);
	program.set_uniform(name, static_cast<int>(texture_unit++));
	bind(target);
}



void basic_texture::update( 
	target::type target,
	format::type format,
	int width, 
	int height, 
	data_format::type data_format,
	data_type::type data_type,
	const void* data, 
	int mipmap_levels ) const
{
    if (GLEW_ARB_texture_storage) {
        glTexStorage2D(target, mipmap_levels, format, width, height);
		if (data)
			glTexSubImage2D(target, 0, 0, 0, width, height, data_format, data_type, data);
    }
    else if (data)
        glTexImage2D(target, 0, format, width, height, 0, data_format, data_type, data);

	if (1 < mipmap_levels)
		glGenerateMipmap(target);
}

void basic_texture::update_compressed( 
	target::type target,
	format::type format,
	int width, 
	int height, 
	int data_size,
	const void* data, 
	int mipmap_levels ) const
{
    if (GLEW_ARB_texture_storage) {
        glTexStorage2D(target, mipmap_levels, format, width, height);
		if (data)
			glCompressedTexSubImage2D(target, 0, 0, 0, width, height, format, data_size, data);
    }
    else if (data)
        glCompressedTexImage2D(target, 0, format, width, height, 0, data_size, data);

	if (1 < mipmap_levels)
		glGenerateMipmap(target);
}

void basic_texture::update( 
	target::type target,
	format::type format,
	int width, 
	int height, 
	int depth, 
	const float* data, 
	int mipmap_levels ) const
{
    if (0 >= mipmap_levels)
	{
		glTexImage3D(target, 0, format, width, height, depth, 0, GL_RGB, GL_FLOAT, data);
		return;
	}

	glTexStorage3D(target, mipmap_levels, format, width, height, depth);
	glTexSubImage3D(target, 0, 0, 0, 0, width, height, depth, GL_RGB, GL_FLOAT, data);
	glGenerateMipmap(target);
}



void basic_texture::generate()
{ glGenTextures(1, &id); }



////////////////////////////////////////////////////////////////////////////////
/// Basic Texture Buffer
////////////////////////////////////////////////////////////////////////////////

void basic_texture_buffer::associate( format::type format ) const
{ glTexBuffer(GL_TEXTURE_BUFFER, format, buffer); }



} // namespace gpu
} // namespace rendering
} // namespace black_label
