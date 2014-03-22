// Version 0.1
#ifndef BLACK_LABEL_PATH_HPP
#define BLACK_LABEL_PATH_HPP

#define BOOST_FILESYSTEM_NO_DEPRECATED
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>



namespace black_label {

using boost::filesystem::path;

inline bool canonical_and_preferred( path& path_, path base )
{ 
	auto unmodified_path = path_;

	boost::system::error_code error;
	path_ = boost::filesystem::canonical(path_, std::move(base), error).make_preferred();

	if (!error)
		return true;
	
	path_ = unmodified_path;
	return false;
}

} // namespace black_label

#endif
