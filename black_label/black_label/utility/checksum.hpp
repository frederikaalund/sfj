#ifndef BLACK_LABEL_UTILITY_CHECKSUM_HPP
#define BLACK_LABEL_UTILITY_CHECKSUM_HPP

#include <boost/crc.hpp>



namespace black_label {
namespace utility {

boost::crc_32_type::value_type crc_32( const void* data, size_t size )
{
	boost::crc_32_type crc;
	crc.process_bytes(data, size);
	return crc.checksum();
}

} // namespace utility
} // namespace black_label



#endif
