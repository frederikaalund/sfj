#ifndef BLACK_LABEL_RENDERER_STORAGE_GPU_TEXTURE_HPP
#define BLACK_LABEL_RENDERER_STORAGE_GPU_TEXTURE_HPP

#include <black_label/renderer/program.hpp>
#include <black_label/renderer/storage/gpu/buffer.hpp>
#include <black_label/renderer/types_and_constants.hpp>

#include <algorithm>
#include <cstdint>
#include <type_traits>

#include <glm/glm.hpp>



namespace black_label {
namespace renderer {
namespace storage {
namespace gpu {

////////////////////////////////////////////////////////////////////////////////
/// Filter and Wrap
////////////////////////////////////////////////////////////////////////////////
enum filter
{
	NEAREST,
	LINEAR,
	MIPMAP
};

enum wrap
{
	CLAMP_TO_EDGE,
	MIRRORED_REPEAT,
	REPEAT
};



////////////////////////////////////////////////////////////////////////////////
/// Texture Base
////////////////////////////////////////////////////////////////////////////////
namespace detail {

class texture_base
{
public:
	struct depth_float { float depth; };
	struct srgb_a { float depth; };

	typedef unsigned int id_type;
	typedef unsigned int target_type;
	
	static const id_type invalid_id = 0;



	friend void swap( texture_base& lhs, texture_base& rhs )
	{
		using std::swap;
		swap(lhs.id, rhs.id);
	}

	texture_base() : id(invalid_id) {};
	texture_base( texture_base&& other ) : id(invalid_id) { swap(*this, other); };
	texture_base( const texture_base& other );
	texture_base( generate_type ) { generate(); };
	~texture_base();
	
	operator id_type() const { return id; }

	bool valid() const { return invalid_id != id; }

	void bind( target_type target ) const;
	void set_parameters( target_type target, filter filter, wrap wrap ) const;
	void use( target_type target, const core_program& program, const char* name, int& texture_unit ) const;

	void update(
		target_type target,
		int width, 
		int height, 
		const std::uint8_t* rgba_data, 
		int mipmap_levels ) const;
	void update(
		target_type target,
		int width, 
		int height, 
		const float* rgba_data, 
		int mipmap_levels ) const;
	void update(
		target_type target,
		int width, 
		int height, 
		const glm::vec3* data, 
		int mipmap_levels ) const;
	void update(
		target_type target,
		int width, 
		int height, 
		const depth_float* data, 
		int mipmap_levels ) const;
	void update(
		target_type target,
		int width, 
		int height, 
		const srgb_a* data, 
		int mipmap_levels ) const;

	void update(
		target_type target,
		int width, 
		int height, 
		int depth, 
		const float* data, 
		int mipmap_levels ) const;

	id_type id;



protected:
	void generate();
};

} // namespace detail



////////////////////////////////////////////////////////////////////////////////
/// Targets
////////////////////////////////////////////////////////////////////////////////
namespace target {
extern detail::texture_base::target_type tex2d_;
extern detail::texture_base::target_type tex3d_;

#ifdef MSVC
detail::texture_base::target_type* tex2d();
detail::texture_base::target_type* tex3d();
#else
constexpr detail::texture_base::target_type* tex2d() { return &tex2d_; }
constexpr detail::texture_base::target_type* tex3d() { return &tex3d_; }
#endif
} // namespace target



////////////////////////////////////////////////////////////////////////////////
/// Texture
////////////////////////////////////////////////////////////////////////////////
template<detail::texture_base::target_type* target()>
class texture : public detail::texture_base
{
public:
	typedef detail::texture_base texture_base;

	friend void swap( texture& lhs, texture& rhs )
	{
		using std::swap;
		swap(static_cast<texture_base&>(lhs), static_cast<texture_base&>(rhs));
	}

	texture() {};
	texture( texture&& other ) { swap(*this, other); };
	texture( generate_type ) : texture_base(black_label::renderer::generate) {};
	texture( filter filter, wrap wrap ) : texture_base(black_label::renderer::generate)
	{ bind(); set_parameters(filter, wrap); }

	template<typename data_type>
	texture( 
		filter filter, 
		wrap wrap, 
		int width, 
		int height, 
		const data_type* rgba_data = nullptr, 
		int mipmap_levels = 8 )
		: detail::texture_base(black_label::renderer::generate)
	{
		bind();
		set_parameters(filter, wrap);
		update(width, height, rgba_data, mipmap_levels);
	}

	template<typename data_type>
	texture( 
		filter filter, 
		wrap wrap, 
		int width, 
		int height, 
		int depth, 
		const data_type* rgb_data = nullptr, 
		int mipmap_levels = 0 )
		: detail::texture_base(black_label::renderer::generate)
	{
		bind();
		set_parameters(filter, wrap);
		update(width, height, depth, rgb_data, mipmap_levels);
	}

	texture& operator=( texture rhs ) { swap(*this, rhs); return *this; }



	void bind() const
	{ texture_base::bind(*target()); }
	void set_parameters( filter filter, wrap wrap ) const
	{ texture_base::set_parameters(*target(), filter, wrap); }
	void use( const core_program& program, const char* name, int& texture_unit ) const
	{ texture_base::use(*target(), program, name, texture_unit); }

	template<typename data_type>
	void update( 
		int width, 
		int height, 
		const data_type* data = nullptr, 
		int mipmap_levels = 8 ) const
	{ texture_base::update(*target(), width, height, data, mipmap_levels); }
	
	template<typename data_type>
	void update( 
		int width, 
		int height, 
		int depth, 
		const data_type* data = nullptr, 
		int mipmap_levels = 8 ) const
	{ texture_base::update(*target(), width, height, depth, data, mipmap_levels); }

	template<typename data_type>
	void bind_and_update( 
		int width, 
		int height, 
		const data_type* rgba_data = nullptr, 
		int mipmap_levels = 8 ) const
	{ bind(); update(width, height, rgba_data, mipmap_levels); }


	

protected:
	texture( const texture& other );
};

typedef texture<target::tex2d> texture_2d;
typedef texture<target::tex3d> texture_3d;



////////////////////////////////////////////////////////////////////////////////
/// Texture Buffer
////////////////////////////////////////////////////////////////////////////////
namespace detail {

class texture_buffer_base
{
public:
	friend void swap( texture_buffer_base& lhs, texture_buffer_base& rhs )
	{
		using std::swap;
		swap(lhs.buffer, rhs.buffer);
		swap(lhs.texture, rhs.texture);
	}

	texture_buffer_base() {};
	texture_buffer_base( texture_buffer_base&& other ) { swap(*this, other); };
	texture_buffer_base( generate_type ) 
		: buffer(generate), texture(generate) {}
	texture_buffer_base( size_t size, const void* data = nullptr )
		: buffer(size, data), texture(generate) {}

	texture_buffer_base& operator=( texture_buffer_base rhs ) { swap(*this, rhs); return *this; }

	bool valid() const { return buffer.valid() && texture.valid(); }

	void associate_as_int32s() const;
	void associate_as_ivec2s() const;
	void associate_as_floats() const;
	void associate_as_vec2s() const;
	void associate_as_vec4s() const;

	void update( size_t size, const void* data = nullptr ) const
	{ buffer.update(size, data); }
	void bind_buffer_and_update( size_t size, const void* data = nullptr ) const
	{ buffer.bind_and_update(size, data); }

	buffer<target::texture, usage::stream_draw> buffer;
	texture<target::texture> texture;



protected:
	texture_buffer_base( const texture_buffer_base& other );
};

} // namespace detail



template<typename data_type_>
class texture_buffer : public detail::texture_buffer_base
{
public:
	typedef data_type_ data_type;

	friend void swap( texture_buffer& lhs, texture_buffer& rhs )
	{
		using std::swap;
		using detail::texture_buffer_base;
		swap(static_cast<texture_buffer_base&>(lhs), static_cast<texture_buffer_base&>(rhs));
	}

	texture_buffer() {};
	texture_buffer( texture_buffer&& other ) { swap(*this, other); };
	texture_buffer( generate_type ) 
		: texture_buffer_base(generate) {}
	texture_buffer( size_t count, const data_type* data = nullptr )
		: detail::texture_buffer_base(size(count), data) {}

	texture_buffer& operator=( texture_buffer rhs ) { swap(*this, rhs); return *this; }

	void associate() const { associate_<data_type>(*this); }

	void update( size_t count, const data_type* data = nullptr ) const
	{ buffer.update(size(count), data); }
	void bind_buffer_and_update( size_t count, const data_type* data = nullptr ) const
	{ buffer.bind_and_update(size(count), data); }

	void use( const core_program& program, const char* name, int& texture_unit ) const
	{ texture.use(program, name, texture_unit); associate(); }

	size_t size( size_t count ) const { return sizeof(data_type) * count; }



protected:
	texture_buffer( const texture_buffer& other );

private:
	template<typename data_type__> static
	typename std::enable_if<std::is_same<data_type__, std::int32_t>::value>::type
	associate_( const texture_buffer<data_type__>& ttb ) { ttb.associate_as_int32s(); }

	template<typename data_type__> static
	typename std::enable_if<std::is_same<data_type__, glm::ivec2>::value>::type
	associate_( const texture_buffer<data_type__>& ttb ) { ttb.associate_as_ivec2s(); }

	template<typename data_type__> static
	typename std::enable_if<std::is_same<data_type__, float>::value>::type
	associate_( const texture_buffer<data_type__>& ttb ) { ttb.associate_as_floats(); }

	template<typename data_type__> static
	typename std::enable_if<std::is_same<data_type__, glm::vec2>::value>::type
	associate_( const texture_buffer<data_type__>& ttb ) { ttb.associate_as_vec2s(); }
    
	template<typename data_type__> static
	typename std::enable_if<std::is_same<data_type__, glm::vec4>::value>::type
	associate_( const texture_buffer<data_type__>& ttb ) { ttb.associate_as_vec4s(); }
};

} // namespace gpu
} // namespace storage
} // namespace renderer
} // namespace black_label



#endif
