#ifndef BLACK_LABEL_RENDERER_STORAGE_GPU_VERTEX_ARRAY_HPP
#define BLACK_LABEL_RENDERER_STORAGE_GPU_VERTEX_ARRAY_HPP

#include <black_label/renderer/types_and_constants.hpp>

#include <algorithm>



namespace black_label {
namespace renderer {
namespace storage {
namespace gpu {

class vertex_array
{
public:
	typedef unsigned int id_type;
	typedef unsigned int index_type;
	static const id_type invalid_id = 0;

	friend void swap( vertex_array& lhs, vertex_array& rhs )
	{
		using std::swap;
		swap(lhs.id, rhs.id);
	}

	vertex_array() : id(invalid_id) {}
	vertex_array( vertex_array&& other ) : id(invalid_id) { swap(*this, other); }
	vertex_array( generate_type ) { generate(); }
	~vertex_array();

	vertex_array& operator=( vertex_array lhs ) { swap(*this, lhs); return *this; }
	operator id_type() const { return id; }

	bool valid() const { return invalid_id != id; }
	
	void bind() const;
	void add_attribute( index_type& index, int size, const void* offset ) const;

	id_type id;



protected:
	vertex_array( const vertex_array& other );

	void generate();
};

} // namespace gpu
} // namespace storage
} // namespace renderer
} // namespace black_label



#endif
