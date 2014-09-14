#ifndef BLACK_LABEL_RENDERER_GPU_BUFFER_HPP
#define BLACK_LABEL_RENDERER_GPU_BUFFER_HPP

#include <black_label/renderer/types_and_constants.hpp>

#include <algorithm>



namespace black_label {
namespace renderer {
namespace gpu {



////////////////////////////////////////////////////////////////////////////////
/// Targets and Usages
////////////////////////////////////////////////////////////////////////////////
namespace target {
	extern const type texture, array, element_array, uniform_buffer, shader_storage;
} // namespace target

namespace usage {
	using type = unsigned int;
	extern const type stream_draw, static_draw, dynamic_draw, dynamic_copy;
} // namespace usage



////////////////////////////////////////////////////////////////////////////////
/// Basic Buffer
////////////////////////////////////////////////////////////////////////////////
class basic_buffer
{
public:
	using id_type = unsigned int;
	using size_type = ptrdiff_t;
	using offset_type = ptrdiff_t;
	static const id_type invalid_id{0};



	friend void swap( basic_buffer& lhs, basic_buffer& rhs )
	{
		using std::swap;
		swap(lhs.id, rhs.id);
	}

	basic_buffer() : id{invalid_id} {};
	basic_buffer( basic_buffer&& other ) : basic_buffer{} { swap(*this, other); };
	basic_buffer( generate_type ) { generate(); }
	basic_buffer( 
		target::type target, 
		usage::type usage,
		size_type size,
		const void* data = nullptr ) 
		: basic_buffer{black_label::renderer::generate}
	{ bind(target); update(target, usage, size, data); }
	~basic_buffer();

	basic_buffer& operator=( basic_buffer rhs ) { swap(*this, rhs); return *this; }

	bool valid() const { return invalid_id != id; }
	void bind( target::type target ) const;
	void update( target::type target, usage::type usage, size_type size, const void* data = nullptr ) const;
	void update( target::type target, offset_type offset, size_type size, const void* data = nullptr ) const;

	operator id_type() const { return id; }

	id_type id;



protected:
	void generate();
};

    

class buffer : protected basic_buffer
{
public:
	using basic_buffer::id_type;
	using basic_buffer::size_type;
	using basic_buffer::offset_type;

	friend void swap( buffer& lhs, buffer& rhs )
	{
		using std::swap;
		swap(static_cast<basic_buffer&>(lhs), static_cast<basic_buffer&>(rhs));
		swap(lhs.target, rhs.target);
		swap(lhs.usage, rhs.usage);
	}

	buffer() {};
	buffer( const buffer& ) = delete;
	buffer( buffer&& other ) : buffer{} { swap(*this, other); };
	buffer( target::type target, usage::type usage ) 
		: basic_buffer{black_label::renderer::generate}
		, target{target} 
		, usage{usage}
	{}
	buffer( 
		target::type target,
		usage::type usage,
		size_type size, 
		const void* data = nullptr ) 
		: basic_buffer{target, usage, size, data} 
		, target{target}
		, usage{usage}
	{}

	buffer& operator=( buffer rhs )	{ swap(*this, rhs); return *this; }

	using basic_buffer::valid;
	using basic_buffer::operator id_type;
	
	void bind() const
	{ basic_buffer::bind(target); }
	void update( size_type size, const void* data = nullptr ) const
	{ basic_buffer::update(target, usage, size, data); }
	void update( offset_type offset, size_type size, const void* data = nullptr ) const
	{ basic_buffer::update(target, offset, size, data); }
	void bind_and_update( size_type size, const void* data = nullptr ) const
	{ bind(); update(size, data); }
	void bind_and_update( offset_type offset, size_type size, const void* data = nullptr ) const
	{ bind(); update(offset, size, data); }

	

	target::type target;
	usage::type usage;
};

} // namespace gpu
} // namespace renderer
} // namespace black_label



#endif
