#ifndef BLACK_LABEL_RENDERING_GPU_TEXTURE_HPP
#define BLACK_LABEL_RENDERING_GPU_TEXTURE_HPP

#include <black_label/rendering/cpu/texture.hpp>
#include <black_label/rendering/program.hpp>
#include <black_label/rendering/gpu/buffer.hpp>
#include <black_label/rendering/types_and_constants.hpp>

#include <algorithm>
#include <cstdint>
#include <type_traits>

#include <glm/glm.hpp>



namespace black_label {
namespace rendering {
namespace gpu {

////////////////////////////////////////////////////////////////////////////////
/// Types and Constants
////////////////////////////////////////////////////////////////////////////////
namespace filter {
	using type = int;
	extern const type nearest, linear, mipmap;
} // namespace filter

namespace format {
	using type = unsigned int;
	extern const type 
		r32i,
		r32f,
		rg32i,
		rgba8, 
		rgba32f, 
		rgba16f, 
		srgb, 
		compressed_srgb, 
		depth16,
		depth24,
		depth32f;

	inline bool is_depth_format( type format )
	{ return depth16 == format || depth24 == format || depth32f == format; }
} // namespace format

namespace wrap {
	using type = int;
	extern const type clamp_to_edge, repeat, mirrored_repeat;
} // namespace wrap

namespace target {
	extern const type texture_2d, texture_3d, texture_buffer;
} // namespace target

namespace data_format {
	using type = unsigned int;
	extern const type rgb, rgba, depth;
} // namespace data_format

namespace data_type {
	using type = unsigned int;
	extern const type uint8, float32;
} // namespace data_type



////////////////////////////////////////////////////////////////////////////////
/// Basic Texture
////////////////////////////////////////////////////////////////////////////////
class basic_texture
{
public:
	typedef unsigned int id_type;
	
	static const id_type invalid_id{0};



	friend void swap( basic_texture& lhs, basic_texture& rhs )
	{
		using std::swap;
		swap(lhs.id, rhs.id);
	}

	basic_texture() : id{invalid_id} {};
	basic_texture( basic_texture&& other ) : basic_texture{} { swap(*this, other); };
	basic_texture( generate_type ) { generate(); };
	basic_texture( target::type target, filter::type filter, wrap::type wrap )
		: basic_texture{black_label::rendering::generate}
	{
		bind(target);
		set_parameters(target, filter, wrap);
	}
	basic_texture( 
		target::type target,
		format::type format,
		filter::type filter, 
		wrap::type wrap, 
		int width, 
		int height,
		data_format::type data_format,
		data_type::type data_type,
		const void* data = nullptr, 
		int mipmap_levels = 1 )
		: basic_texture{target, filter, wrap}
	{ update(target, format, width, height, data_format, data_type, data, mipmap_levels); }
	template<typename data_type>
	basic_texture( 
		target::type target,
		format::type format,
		filter::type filter, 
		wrap::type wrap, 
		int width, 
		int height,
		const data_type* data = nullptr, 
		int mipmap_levels = 1 )
		: basic_texture{target, filter, wrap}
	{ update(target, format, width, height, data, mipmap_levels); }

	~basic_texture();



	basic_texture& operator=( basic_texture rhs ) { swap(*this, rhs); return *this; }
	operator id_type() const { return id; }



	bool valid() const { return invalid_id != id; }

	void bind( target::type target ) const;
	void set_parameters( target::type target, filter::type filter, wrap::type wrap ) const;
	void use( target::type target, const core_program& program, const char* name, unsigned int& texture_unit ) const;

	void update(
		target::type target,
		format::type format,
		int width, 
		int height,
		data_format::type data_format,
		data_type::type data_type,
		const void* data, 
		int mipmap_levels ) const;
	
	void update_compressed(
		target::type target,
		format::type format,
		int width, 
		int height,
		int data_size,
		const void* data, 
		int mipmap_levels ) const;

	void update(
		target::type target,
		format::type format,
		int width, 
		int height,
		const std::uint8_t* rgba_data, 
		int mipmap_levels ) const
	{
		update(target, format, width, height,
			data_format::rgba, data_type::uint8, 
			rgba_data, mipmap_levels);
	}

	void update(
		target::type target,
		format::type format,
		int width, 
		int height, 
		const float* rgba_data, 
		int mipmap_levels ) const
	{
		update(target, format, width, height,
			data_format::rgba, data_type::float32, 
			rgba_data, mipmap_levels);
	}

	void update(
		target::type target,
		format::type format,
		int width, 
		int height, 
		const glm::vec3* data, 
		int mipmap_levels ) const
	{
		update(target, format, width, height,
			data_format::rgb, data_type::float32, 
			data, mipmap_levels);
	}

	void update(
		target::type target,
		format::type format,
		int width, 
		int height, 
		int depth, 
		const float* data, 
		int mipmap_levels ) const;



	id_type id;

protected:
	void generate();
};



////////////////////////////////////////////////////////////////////////////////
/// Texture
////////////////////////////////////////////////////////////////////////////////
class texture : protected basic_texture
{
public:
	friend void swap( texture& lhs, texture& rhs )
	{
		using std::swap;
		swap(static_cast<basic_texture&>(lhs), static_cast<basic_texture&>(rhs));
		swap(lhs.target, rhs.target);
		swap(lhs.format, rhs.format);
		swap(lhs.checksum, rhs.checksum);
	}

	texture() : basic_texture{} {};
	texture( texture&& other ) : texture{} { swap(*this, other); };
	texture( 
		target::type target, 
		format::type format, 
		filter::type filter,
		wrap::type wrap ) 
		: basic_texture{target, filter, wrap}
		, target{target}
		, format{format}
	{}
	texture( 
		target::type target,
		format::type format,
		filter::type filter, 
		wrap::type wrap, 
		int width, 
		int height,
		data_format::type data_format,
		data_type::type data_type,
		const void* data = nullptr, 
		int mipmap_levels = 1 )
		: basic_texture{target, format, filter, wrap, width, height, data_format, data_type, data, mipmap_levels}
		, target{target}
		, format{format}
	{}
	template<typename data_type>
	texture( 
		target::type target,
		format::type format,
		filter::type filter, 
		wrap::type wrap, 
		int width, 
		int height,
		const data_type* data = nullptr, 
		int mipmap_levels = 1 )
		: basic_texture{target, format, filter, wrap, width, height, data, mipmap_levels}
		, target{target}
		, format{format}
	{}
	texture( const cpu::texture& cpu_texture )
		: basic_texture{
			target::texture_2d, 
			filter::mipmap, 
			wrap::repeat}
		, target{target::texture_2d}
		, checksum{cpu_texture.checksum}
	{
		if (cpu_texture.compressed)
		{
			format = format::compressed_srgb;
			basic_texture::update_compressed(
				target, 
				format,
				cpu_texture.width,
				cpu_texture.height,
				static_cast<int>(cpu_texture.data.size()),
				cpu_texture.data.data(),
				8);
		}
		else
		{
			format = format::srgb;
			basic_texture::update(
				target, 
				format,
				cpu_texture.width,
				cpu_texture.height,
				data_format::rgba,
				data_type::uint8,
				cpu_texture.data.data(),
				8);
		}
	}

	texture& operator=( texture rhs ) { swap(*this, rhs); return *this; }
	using basic_texture::operator basic_texture::id_type;

	using basic_texture::valid;
	bool has_depth_format() const
	{ return format::is_depth_format(format); }

	void bind() const
	{ basic_texture::bind(target); }
	void set_parameters( filter::type filter, wrap::type wrap ) const
	{ basic_texture::set_parameters(target, filter, wrap); }
	void use( const core_program& program, const char* name, unsigned int& texture_unit ) const
	{ basic_texture::use(target, program, name, texture_unit); }

	void update(
		int width, 
		int height,
		data_format::type data_format = data_format::type{},
		data_type::type data_type = data_type::type{},
		const void* data = nullptr, 
		int mipmap_levels = 1 ) const
	{ basic_texture::update(target, format, width, height, data_format, data_type, data, mipmap_levels); }
	template<typename data_type>
	void update(
		int width, 
		int height,
		const data_type* data = nullptr, 
		int mipmap_levels = 1 ) const
	{ basic_texture::update(target, format, width, height, data, mipmap_levels); }

	void bind_and_update(
		int width, 
		int height,
		data_format::type data_format = data_format::type{},
		data_type::type data_type = data_type::type{},
		const void* data = nullptr, 
		int mipmap_levels = 1 ) const
	{ bind(); update(width, height, data_format, data_type, data, mipmap_levels); }
	template<typename data_type>
	void bind_and_update(
		int width, 
		int height,
		const data_type* data = nullptr, 
		int mipmap_levels = 1 ) const
	{ bind(); update(width, height, data, mipmap_levels); }



	target::type target;
	format::type format;
	utility::checksum checksum;
};



////////////////////////////////////////////////////////////////////////////////
/// Storage Texture
////////////////////////////////////////////////////////////////////////////////
class storage_texture : public texture {
public:

	friend void swap( storage_texture& lhs, storage_texture& rhs ) {
		using std::swap;
		swap(static_cast<texture&>(lhs), static_cast<texture&>(rhs));
		swap(lhs.filter, rhs.filter);
		swap(lhs.wrap, rhs.wrap);
		swap(lhs.width, rhs.width);
		swap(lhs.height, rhs.height);
	}

	storage_texture() {}
	storage_texture( storage_texture&& other ) : storage_texture{} { swap(*this, other); }
	storage_texture& operator=( storage_texture rhs ) { swap(*this, rhs); return *this; }
	storage_texture( 
		target::type target, 
		format::type format, 
		filter::type filter,
		wrap::type wrap,
		float width,
		float height ) 
		: texture{target, format, filter, wrap}
		, filter{filter}
		, wrap{wrap}
		, width{width}
		, height{height}
	{}
	storage_texture( 
		const storage_texture& other,
		int width, 
		int height )
		: texture{other.target, other.format, other.filter, other.wrap, static_cast<int>(other.width * width), static_cast<int>(other.height * height), data_format::rgb, data_type::uint8}
		, filter{other.filter}
		, wrap{other.wrap}
		, width{other.width}
		, height{other.height}
	{}
	
	filter::type filter;
	wrap::type wrap;
	float width, height;
};



////////////////////////////////////////////////////////////////////////////////
/// Basic Texture Buffer
////////////////////////////////////////////////////////////////////////////////
class basic_texture_buffer
{
public:
	friend void swap( basic_texture_buffer& lhs, basic_texture_buffer& rhs )
	{
		using std::swap;
		swap(lhs.buffer, rhs.buffer);
		swap(lhs.texture, rhs.texture);
	}



	basic_texture_buffer() {};
	basic_texture_buffer( const basic_texture_buffer& ) = delete;
	basic_texture_buffer( basic_texture_buffer&& other ) { swap(*this, other); };
	basic_texture_buffer( usage::type usage ) 
		: buffer{target::texture, usage}, texture{generate} {}
	basic_texture_buffer( 
		usage::type usage,
		buffer::size_type size, 
		const void* data = nullptr )
		: buffer{target::texture, usage, size, data}, texture{generate} {}

	basic_texture_buffer& operator=( basic_texture_buffer rhs ) { swap(*this, rhs); return *this; }

	bool valid() const { return buffer.valid() && texture.valid(); }

	void associate( format::type format ) const;

	void update( size_t size, const void* data = nullptr ) const
	{ buffer.update(size, data); }
	void bind_buffer_and_update( size_t size, const void* data = nullptr ) const
	{ buffer.bind_and_update(size, data); }

	buffer buffer;
	basic_texture texture;
};



////////////////////////////////////////////////////////////////////////////////
/// Texture Buffer
////////////////////////////////////////////////////////////////////////////////
class texture_buffer : public basic_texture_buffer
{
public:
	friend void swap( texture_buffer& lhs, texture_buffer& rhs )
	{
		using std::swap;
		swap(static_cast<basic_texture_buffer&>(lhs), static_cast<basic_texture_buffer&>(rhs));
		swap(lhs.format, rhs.format);
	}



	texture_buffer() {};
	texture_buffer( texture_buffer&& other ) { swap(*this, other); };
	texture_buffer( usage::type usage, format::type format ) 
		: basic_texture_buffer{usage}
		, format{format}
	{}
	template<typename data_type>
	texture_buffer(
		usage::type usage,
		format::type format, 
		buffer::size_type count, 
		const data_type* data = nullptr )
		: basic_texture_buffer{
			usage, 
			size<data_type>(count), 
			data}
		, format{format}
	{}

	texture_buffer& operator=( texture_buffer rhs ) { swap(*this, rhs); return *this; }



	void associate() const { basic_texture_buffer::associate(format); }

	template<typename data_type>
	void update( buffer::size_type count, const data_type* data = nullptr ) const
	{ buffer.update(size<data_type>(count), data); }

	template<typename data_type>
	void bind_buffer_and_update( buffer::size_type count, const data_type* data = nullptr ) const
	{ buffer.bind_and_update(size<data_type>(count), data); }

	void use( const core_program& program, const char* name, unsigned int& texture_unit ) const
	{ texture.use(target::texture_buffer, program, name, texture_unit); associate(); }

	template<typename data_type>
	buffer::size_type size( buffer::size_type count ) const { return sizeof(data_type) * count; }



	format::type format;
};

} // namespace gpu
} // namespace rendering
} // namespace black_label



#endif
