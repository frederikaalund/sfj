#ifndef BLACK_LABEL_RENDERER_GPU_FRAMEBUFFER_HPP
#define BLACK_LABEL_RENDERER_GPU_FRAMEBUFFER_HPP

#include <black_label/renderer/types_and_constants.hpp>

#include <algorithm>



namespace black_label {
namespace renderer {
namespace gpu {



////////////////////////////////////////////////////////////////////////////////
/// Basic Framebuffer
////////////////////////////////////////////////////////////////////////////////
class basic_framebuffer {
public:
	using id_type = unsigned int;
	static const id_type invalid_id{0};



	friend void swap( basic_framebuffer& lhs, basic_framebuffer& rhs )
	{
		using std::swap;
		swap(lhs.id, rhs.id);
	}

	basic_framebuffer() : id{invalid_id} {}
	basic_framebuffer( basic_framebuffer&& other ) : basic_framebuffer{} { swap(*this, other); }
	basic_framebuffer( generate_type ) { generate(); }
	~basic_framebuffer();

	basic_framebuffer& operator=( basic_framebuffer rhs ) { swap(*this, rhs); return *this; }

	void bind() const;
	bool valid() const { return invalid_id != id; }

	operator id_type() const { return id; }

	id_type id;



protected:
	void generate();
};


using framebuffer = basic_framebuffer;

} // namespace gpu
} // namespace renderer
} // namespace black_label



#endif
