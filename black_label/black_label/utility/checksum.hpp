#ifndef BLACK_LABEL_UTILITY_CHECKSUM_HPP
#define BLACK_LABEL_UTILITY_CHECKSUM_HPP

#include <black_label/file_buffer.hpp>
#include <black_label/path.hpp>

#include <fstream>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/crc.hpp>
#include <boost/serialization/access.hpp>



namespace black_label {
namespace utility {

inline boost::crc_32_type::value_type crc_32( const void* data, size_t size )
{
	boost::crc_32_type crc;
	crc.process_bytes(data, size);
	return crc.checksum();
}



struct from_binary_header_type {};
const from_binary_header_type from_binary_header;

class checksum
{
public:
	using checksum_type = boost::crc_32_type::value_type;

	friend class boost::serialization::access;



	checksum() : value(0) {}
	explicit checksum( path path ) : checksum{} 
	{
		file_buffer::file_buffer model_file{path.string()};
		if (model_file.empty()) return;

		value = black_label::utility::crc_32(model_file.data(), model_file.size());
			return;
	}
	checksum( std::ifstream& stream, from_binary_header_type ) 
	{	
		using namespace boost;
		using namespace boost::archive;

		assert(stream.is_open());

		class header
		{
		public:
			friend class boost::serialization::access;

			void serialize( binary_iarchive& archive, unsigned int version )
			{ archive & checksum; }

			checksum checksum;
		};

		header header;
		boost::archive::binary_iarchive{stream} >> header;
		value = header.checksum.value;
		stream.seekg(0);
	}
	checksum( const checksum& ) = default;

	explicit operator bool() const { return 0 != value; }
	bool operator==( checksum rhs ) const { return value == rhs.value; }
	bool operator!=( checksum rhs) const { return !operator==(rhs); }



	template<typename archive_type>
	void serialize( archive_type& archive, unsigned int version )
	{ archive & value; }



	checksum_type value;
};

} // namespace utility
} // namespace black_label



#endif
