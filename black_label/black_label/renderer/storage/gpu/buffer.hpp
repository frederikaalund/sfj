#ifndef BLACK_LABEL_RENDERER_STORAGE_GPU_BUFFER_HPP
#define BLACK_LABEL_RENDERER_STORAGE_GPU_BUFFER_HPP

#include <black_label/renderer/types_and_constants.hpp>

#include <algorithm>



namespace black_label {
namespace renderer {
namespace storage {
namespace gpu {



namespace detail {

////////////////////////////////////////////////////////////////////////////////
/// Buffer Base
////////////////////////////////////////////////////////////////////////////////
class buffer_base
{
public:
	typedef unsigned int id_type;
	typedef unsigned int target_type;
	typedef unsigned int usage_type;
	typedef ptrdiff_t size_type;
	typedef ptrdiff_t offset_type;
	static const id_type invalid_id = 0;



	friend void swap( buffer_base& lhs, buffer_base& rhs )
	{
		using std::swap;
		swap(lhs.id, rhs.id);
	}

	buffer_base() : id(invalid_id) {};
	buffer_base( buffer_base&& other ) : id(invalid_id) { swap(*this, other); };
	buffer_base( generate_type ) { generate(); }
	~buffer_base();

	buffer_base& operator=( buffer_base rhs ) { swap(*this, rhs); return *this; }

	bool valid() const { return invalid_id != id; }
	void bind( target_type target ) const;
	void update( target_type target, usage_type usage, size_type size, const void* data = nullptr ) const;
	void update( target_type target, offset_type offset, size_type size, const void* data = nullptr ) const;

	operator id_type() const { return id; }

	id_type id;



protected:
	buffer_base( const buffer_base& other );

	void generate();
};

} // namespace detail



////////////////////////////////////////////////////////////////////////////////
/// Targets and Usages
////////////////////////////////////////////////////////////////////////////////
namespace target {
struct texture { static detail::buffer_base::target_type get(); };
struct array { static detail::buffer_base::target_type get(); };
struct element_array { static detail::buffer_base::target_type get(); };
} // namespace targets

namespace usage {
struct stream_draw { static detail::buffer_base::usage_type get(); };
struct static_draw { static detail::buffer_base::usage_type get(); };
} // namespace usages



////////////////////////////////////////////////////////////////////////////////
/// Buffer
////////////////////////////////////////////////////////////////////////////////
template<typename target_type, typename usage_type>
class buffer : public detail::buffer_base
{
public:
	typedef detail::buffer_base buffer_base;

	friend void swap( buffer& lhs, buffer& rhs )
	{
		using std::swap;
		swap(static_cast<buffer_base&>(lhs), static_cast<buffer_base&>(rhs));
	}

	buffer() {};
	buffer( buffer&& other ) { swap(*this, other); };
	buffer( generate_type ) : buffer_base(black_label::renderer::generate) {}
	buffer( size_type size, const void* data = nullptr ) : buffer_base(black_label::renderer::generate)
	{ bind_and_update(size, data); }

	buffer& operator=( buffer rhs )	{ swap(*this, rhs); return *this; }
	
	void bind() const
	{ buffer_base::bind(target_type::get()); }
	void update( size_type size, const void* data = nullptr ) const
	{ buffer_base::update(target_type::get(), usage_type::get(), size, data); }
	void update( offset_type offset, size_type size, const void* data = nullptr ) const
	{ buffer_base::update(target_type::get(), offset, size, data); }
	void bind_and_update( size_type size, const void* data = nullptr ) const
	{ bind(); update(size, data); }
	void bind_and_update( offset_type offset, size_type size, const void* data = nullptr ) const
	{ bind(); update(offset, size, data); }


protected:
	buffer( const buffer& other );
};

} // namespace gpu
} // namespace storage
} // namespace renderer
} // namespace black_label



#endif
