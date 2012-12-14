#ifndef BLACK_LABEL_UTILITY_ALGORITHM_HPP
#define BLACK_LABEL_UTILITY_ALGORITHM_HPP

#include <algorithm>



namespace black_label {
namespace utility {

////////////////////////////////////////////////////////////////////////////////
/// Minimum Element
/// 
/// Finds the minimum element of a range and returns an iterator pointing to 
/// it. Unlike std::min_element, utility::min_element does not require the
/// Predicate to have strict ordering (the case of VS2010 in debug). This is 
/// useful when strict ordering can not be guaranteed all the time but applies 
/// most of the time anyways. E.g. when concurrency is involved.
////////////////////////////////////////////////////////////////////////////////
template<typename iterator, typename predicate>
iterator min_element( iterator first, iterator last, predicate predicate_ )
{ 
	iterator result = first;
	if (first != last)
		while (++first != last)
			if (predicate_(*first, *result))
				result = first;
	return result;
}

} // namespace utility
} // namespace black_label



#endif
