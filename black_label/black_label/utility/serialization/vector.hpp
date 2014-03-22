#ifndef BLACK_LABEL_UTILITY_SERIALIZATION_VECTOR_HPP
#define BLACK_LABEL_UTILITY_SERIALIZATION_VECTOR_HPP

#include <vector>

#include <boost/serialization/split_free.hpp>



namespace boost {
namespace serialization {

template<typename archive_type, typename T, typename allocator_type>
void save( archive_type& archive, const std::vector<T, allocator_type>& vector, unsigned int version )
{
	auto size = vector.size();
	archive & size;
	archive & make_array(vector.data(), size);
}

template<typename archive_type, typename T, typename allocator_type>
void load( archive_type& archive, std::vector<T, allocator_type>& vector, unsigned int version )
{
	typename std::vector<T, allocator_type>::size_type size;
	archive & size;
	vector.resize(size);
	archive & make_array(vector.data(), size);
}

template<typename archive_type, typename T, typename allocator_type>
void serialize( archive_type& archive, std::vector<T, allocator_type>& vector, unsigned int version )
{
	split_free(archive, vector, version);
}

} // namespace serialization
} // namespace boost



#endif
