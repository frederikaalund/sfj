#ifndef BLACK_LABEL_UTILITY_CACHE_FILE_HPP
#define BLACK_LABEL_UTILITY_CACHE_FILE_HPP

#include <black_label/utility/checksum.hpp>
#include <black_label/path.hpp>

#include <fstream>
#include <utility>

#include <boost/log/trivial.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/access.hpp>



namespace black_label {
namespace utility {



class cache_file
{
public:
	friend void swap( cache_file& lhs, cache_file& rhs )
	{
		using std::swap;
		swap(lhs.checksum, rhs.checksum);
	}

	cache_file() {}
	cache_file( path path ) : checksum(std::move(path)) {}
	cache_file( cache_file&& other ) { swap(*this, other); }
	cache_file& operator=( cache_file rhs ) { swap(*this, rhs); return *this; }



	void apply_extension( path& path )
	{ if (".cache" != path.extension()) path += ".cache"; }

	template<typename derived>
	bool import( path path, derived& derived )
	{
		apply_extension(path);

		std::ifstream file{path.string(), std::ifstream::binary};
		if (!file.is_open()) return false;
	
		BOOST_LOG_TRIVIAL(info) << "Importing cache file " << path;

		if (utility::checksum{file, from_binary_header} != checksum)
		{
			BOOST_LOG_TRIVIAL(info) << "Cache file is outdated " << path;
			return false;
		}

		file.seekg(0);
		try { boost::archive::binary_iarchive{file} >> derived; }
		catch (std::exception e)
		{
			BOOST_LOG_TRIVIAL(error) << e.what();
			return false;
		}

		BOOST_LOG_TRIVIAL(info) << "Imported cache file " << path;
		return true;
	}

	template<typename derived>
	bool export( path path, const derived& derived )
	{
		BOOST_LOG_TRIVIAL(info) << "Exporting cache file " << path;

		apply_extension(path);
	
		std::ofstream file{path.string(), std::ofstream::binary};
		if (!file.is_open())
			return false;
	
		try { boost::archive::binary_oarchive{file} << derived; }
		catch (std::exception e)
		{
			BOOST_LOG_TRIVIAL(error) << e.what();
			return false;
		}

		BOOST_LOG_TRIVIAL(info) << "Exported cache file " << path;
		return true;
	}



	utility::checksum checksum;
};



} // namespace utility
} // namespace black_label



#endif
