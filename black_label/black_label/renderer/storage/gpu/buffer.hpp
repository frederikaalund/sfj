#ifndef BLACK_LABEL_RENDERER_STORAGE_GPU_BUFFER_HPP
#define BLACK_LABEL_RENDERER_STORAGE_GPU_BUFFER_HPP

#include <black_label/renderer/storage/gpu/detail/buffer_base.hpp>
#include <black_label/renderer/types_and_constants.hpp>

#include <algorithm>



namespace black_label {
namespace renderer {
namespace storage {
namespace gpu {

template<detail::buffer_base::target_type target(), detail::buffer_base::usage_type usage()>
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
	{ buffer_base::bind(target()); }
	void update( size_type size, const void* data = nullptr ) const
	{ buffer_base::update(target(), usage(), size, data); }
	void update( offset_type offset, size_type size, const void* data = nullptr ) const
	{ buffer_base::update(target(), offset, size, data); }
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
