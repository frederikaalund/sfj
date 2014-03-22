#ifndef BLACK_LABEL_UTILITY_SERIALIZATION_PATH_HPP
#define BLACK_LABEL_UTILITY_SERIALIZATION_PATH_HPP

#include <black_label/path.hpp>



namespace boost {
namespace serialization {

template<typename archive_type>
void save(archive_type& archive, const boost::filesystem::path& path, unsigned int version)
{ archive & path.string(); }

template<typename archive_type>
void load(archive_type& archive, boost::filesystem::path& path, unsigned int version)
{
	std::string string;
	archive & string;
	path = boost::filesystem::path{string};
}

template<typename archive_type>
void serialize(archive_type& archive, boost::filesystem::path& path, unsigned int version)
{ split_free(archive, path, version); }

} // namespace serialization
} // namespace boost



#endif
