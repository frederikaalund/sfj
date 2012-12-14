#ifndef BLACK_LABEL_UTILITY_SERIALIZATION_ARRAY_HPP
#define BLACK_LABEL_UTILITY_SERIALIZATION_ARRAY_HPP

#include <array>



namespace boost {
namespace serialization {

template<typename archive_type, typename T, size_t size>
void serialize( archive_type& archive, std::array<T, size>& array, unsigned int version )
{
	archive & make_array(array.data(), size);
}

} // namespace serialization
} // namespace boost



#endif
