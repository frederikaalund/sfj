#ifndef BLACK_LABEL_UTILITY_RANGE_HPP
#define BLACK_LABEL_UTILITY_RANGE_HPP

#include <iterator>

#include <boost/range/iterator_range.hpp>



namespace black_label {
namespace utility {



////////////////////////////////////////////////////////////////////////////////
/// Pointer Range
////////////////////////////////////////////////////////////////////////////////
template<typename T>
class pointer_range : public boost::iterator_range<T*>
{
public:
	using boost::iterator_range<T*>::iterator_range;
	pointer_range() {}
	template<typename U>
	pointer_range( std::initializer_list<U> list ) 
		: boost::iterator_range<T*>(list) {}
};



} // namespace utility
} // namespace black_label



#endif
