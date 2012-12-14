#ifndef BLACK_LABEL_UTILITY_BOOST_ATOMIC_EXTENSIONS_HPP
#define BLACK_LABEL_UTILITY_BOOST_ATOMIC_EXTENSIONS_HPP

#include <boost/atomic.hpp>



namespace std {

template<typename T>
void swap( boost::atomic<T>& lhs, boost::atomic<T>& rhs )
{ 
	lhs.store(rhs.exchange(lhs.load())); 
}

} // namespace std



#endif
