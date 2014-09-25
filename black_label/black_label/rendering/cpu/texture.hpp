#ifndef BLACK_LABEL_RENDERING_CPU_TEXTURE_HPP
#define BLACK_LABEL_RENDERING_CPU_TEXTURE_HPP

#include <black_label/utility/cache_file.hpp>
#include <black_label/utility/serialization/vector.hpp>

#include <cstdint>

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>



namespace black_label {
namespace rendering {

struct defer_import_type {};
const defer_import_type defer_import;

namespace cpu {

class texture : public utility::cache_file
{
public:
	using data_container = std::vector<std::uint8_t>;
	using size_type = int;

	friend class boost::serialization::access;



	friend void swap( texture& lhs, texture& rhs )
	{
		using std::swap;
		swap(static_cast<cache_file&>(lhs), static_cast<cache_file&>(rhs));
		swap(lhs.data, rhs.data);
		swap(lhs.width, rhs.width);
		swap(lhs.height, rhs.height);
		swap(lhs.compressed, rhs.compressed);
	}
	
	texture() : compressed{false} {}
	texture( path path, defer_import_type ) : cache_file{std::move(path)}, compressed{false} {}
	explicit texture( path path ) : texture{path, defer_import} { import(std::move(path)); }
	texture( const texture& ) = delete;
	texture( texture&& other ) : texture{} { swap(*this, other); }
	texture& operator=( texture rhs ) { swap(*this, rhs); return *this; }

	bool import( path path );
#ifdef DEVELOPER_TOOLS
	bool import_sfml( path path );
	void compress();
#endif // #ifdef DEVELOPER_TOOLS

	bool is_empty() const { return data.empty(); }
	explicit operator bool() const { return !is_empty(); }



	template<typename archive_type>
	void serialize( archive_type& archive, unsigned int version )
	{ archive & checksum & data & width & height & compressed; }

	data_container data;
	size_type width, height;
	bool compressed;
};



} // namespace cpu
} // namespace rendering
} // namespace black_label



#endif
