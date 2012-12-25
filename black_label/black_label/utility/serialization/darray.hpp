#ifndef BLACK_LABEL_UTILITY_SERIALIZATION_DARRAY_HPP
#define BLACK_LABEL_UTILITY_SERIALIZATION_DARRAY_HPP

#include <black_label/container/darray.hpp>

#include <boost/serialization/array.hpp>
#include <boost/serialization/split_free.hpp>



namespace boost {
namespace serialization {

template<typename archive_type, typename T>
void save( archive_type& archive, const black_label::container::darray<T>& darray, unsigned int version )
{
	typename black_label::container::darray<T>::size_type capacity = darray.capacity();
	archive << capacity;
	archive << make_array(darray.data(), capacity);
}

template<typename archive_type, typename T>
void load( archive_type& archive, black_label::container::darray<T>& darray, unsigned int version )
{
	typedef black_label::container::darray<T> darray_type;

	typename darray_type::size_type capacity;
	archive >> capacity;
	darray = darray_type(capacity);
	archive >> make_array(darray.data(), capacity);
}

template<typename archive_type, typename T>
void serialize( archive_type& archive, black_label::container::darray<T>& darray, unsigned int version )
{
	split_free(archive, darray, version);
}

} // namespace serialization
} // namespace boost



#endif
