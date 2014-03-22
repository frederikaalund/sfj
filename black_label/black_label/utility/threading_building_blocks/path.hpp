#ifndef BLACK_LABEL_UTILITY_THREADING_BUILDING_BLOCKS_PATH_HPP
#define BLACK_LABEL_UTILITY_THREADING_BUILDING_BLOCKS_PATH_HPP

#include <black_label/path.hpp>

#include <boost/functional/hash.hpp>



namespace std {

template<> 
struct hash<boost::filesystem::path>
{
	size_t operator()( const boost::filesystem::path& p ) const
	{ return boost::filesystem::hash_value(p); }
};

} // namespace std


namespace tbb {

inline size_t tbb_hasher( const boost::filesystem::path& path_ )
{ return boost::filesystem::hash_value(path_); }

} // namespace tbb



#endif
