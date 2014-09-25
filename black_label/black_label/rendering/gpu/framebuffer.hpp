#ifndef BLACK_LABEL_RENDERING_GPU_FRAMEBUFFER_HPP
#define BLACK_LABEL_RENDERING_GPU_FRAMEBUFFER_HPP

#include <black_label/rendering/gpu/texture.hpp>
#include <black_label/rendering/types_and_constants.hpp>

#include <algorithm>
#include <vector>



namespace black_label {
namespace rendering {
namespace gpu {



////////////////////////////////////////////////////////////////////////////////
/// Basic Framebuffer
////////////////////////////////////////////////////////////////////////////////
class basic_framebuffer {
public:
	using id_type = unsigned int;
	using attachment_type = unsigned int;
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
	static void unbind();
	void attach_texture_2d( attachment_type attachment, basic_texture::id_type texture ) const;
	static void detach( attachment_type attachment );
	void set_draw_buffers( int size, const attachment_type* buffers ) const;
	bool valid() const { return invalid_id != id; }
	bool is_complete() const;

	operator id_type() const { return id; }

	id_type id;



protected:
	void generate();
};



////////////////////////////////////////////////////////////////////////////////
/// Framebuffer
////////////////////////////////////////////////////////////////////////////////
class framebuffer : public basic_framebuffer {
public:
	using basic_framebuffer::basic_framebuffer;

	template<typename range>
	void attach( const range& textures )
	{
		// Detach any previously bound attachments
		for (auto attachment : draw_buffers) detach(attachment);
		draw_buffers.clear();
		
		// Attach provided arguments
		attachment_type attachment_index{0};
		for (const gpu::texture& texture : textures) {
			auto attachment = attach(texture, attachment_index);
			if (!texture.has_depth_format())
				draw_buffers.push_back(attachment);
		}

		set_draw_buffers();
	}

	attachment_type attach( const texture& texture, attachment_type& attachment_index ) const;
	void set_draw_buffers() const
	{ basic_framebuffer::set_draw_buffers(static_cast<int>(draw_buffers.size()), draw_buffers.data()); }

	std::vector<attachment_type> draw_buffers;
};

} // namespace gpu
} // namespace rendering
} // namespace black_label



#endif
