#ifndef BLACK_LABEL_RENDERING_GPU_BUFFER_HPP
#define BLACK_LABEL_RENDERING_GPU_BUFFER_HPP

#include <black_label/rendering/types_and_constants.hpp>

#include <algorithm>



namespace black_label {
namespace rendering {
namespace gpu {



////////////////////////////////////////////////////////////////////////////////
/// Targets and Usages
////////////////////////////////////////////////////////////////////////////////
namespace target {
	extern const type texture, array, element_array, uniform_buffer, 
		shader_storage, atomic_counter;
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
	using index_type = unsigned int;
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
		: basic_buffer{black_label::rendering::generate}
	{ bind(target); update(target, usage, size, data); }
	~basic_buffer();

	basic_buffer& operator=( basic_buffer rhs ) { swap(*this, rhs); return *this; }

	bool valid() const { return invalid_id != id; }
	void bind( target::type target ) const;
	void bind( target::type target, index_type index ) const;
	static void unbind( target::type target );
	static void unbind();
	void update( target::type target, usage::type usage, size_type size, const void* data = nullptr ) const;
	void update( target::type target, offset_type offset, size_type size, const void* data = nullptr ) const;

	operator id_type() const { return id; }

	id_type id;



protected:
	void generate();
};

    

////////////////////////////////////////////////////////////////////////////////
/// Buffer
////////////////////////////////////////////////////////////////////////////////
class buffer : public basic_buffer
{
public:
	using basic_buffer::id_type;
	using basic_buffer::size_type;
	using basic_buffer::offset_type;
	using basic_buffer::index_type;

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
		: basic_buffer{black_label::rendering::generate}
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
	void bind( index_type index ) const
	{ basic_buffer::bind(target, index); }
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



////////////////////////////////////////////////////////////////////////////////
/// Index-bound Buffer
////////////////////////////////////////////////////////////////////////////////
class index_bound_buffer : public buffer
{
public:
	using buffer::id_type;
	using buffer::size_type;
	using buffer::offset_type;
	using buffer::index_type;
	static const index_type invalid_index{-1};

	friend void swap( index_bound_buffer& lhs, index_bound_buffer& rhs )
	{
		using std::swap;
		swap(static_cast<buffer&>(lhs), static_cast<buffer&>(rhs));
		swap(lhs.index, rhs.index);
	}

	index_bound_buffer() : index{invalid_index} {};
	index_bound_buffer( index_bound_buffer&& other ) : index_bound_buffer{} { swap(*this, other); };
	index_bound_buffer( target::type target, usage::type usage, index_type index ) 
		: buffer{target, usage}
		, index{index} 
	{}
	index_bound_buffer( 
		target::type target,
		usage::type usage,
		index_type index,
		size_type size, 
		const void* data = nullptr ) 
		: buffer{target, usage, size, data} 
		, index{index}
	{}
	index_bound_buffer& operator=( index_bound_buffer rhs )	{ swap(*this, rhs); return *this; }

	void bind() const { buffer::bind(index); }

	index_type index;
};



} // namespace gpu
} // namespace rendering
} // namespace black_label



#endif
