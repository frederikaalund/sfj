#ifndef BLACK_LABEL_RENDERER_STORAGE_GPU_DETAIL_BUFFER_BASE_HPP
#define BLACK_LABEL_RENDERER_STORAGE_GPU_DETAIL_BUFFER_BASE_HPP

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
extern detail::buffer_base::target_type texture_;
extern detail::buffer_base::target_type array_;
extern detail::buffer_base::target_type element_array_;
    
#ifdef MSVC
detail::buffer_base::target_type* texture();
detail::buffer_base::target_type* array();
detail::buffer_base::target_type* element_array();
#else
constexpr detail::buffer_base::target_type* texture() { return &texture_; };
constexpr detail::buffer_base::target_type* array() { return &array_; };
constexpr detail::buffer_base::target_type* element_array() { return &element_array_; };
#endif
} // namespace target

namespace usage {
extern detail::buffer_base::usage_type stream_draw_;
extern detail::buffer_base::usage_type static_draw_;
    
#ifdef MSVC
detail::buffer_base::target_type* stream_draw();
detail::buffer_base::target_type* static_draw();
#else
constexpr detail::buffer_base::target_type* stream_draw() { return &stream_draw_; };
constexpr detail::buffer_base::target_type* static_draw() { return &static_draw_; };
#endif
} // namespace usage
    
    
    
} // namespace gpu
} // namespace storage
} // namespace renderer
} // namespace black_label



#endif
