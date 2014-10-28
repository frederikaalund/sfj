// Version 0.1
#ifndef BLACK_LABEL_PATH_HPP
#define BLACK_LABEL_PATH_HPP

#include <black_label/utility/algorithm.hpp>

#define BOOST_FILESYSTEM_NO_DEPRECATED
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>



namespace black_label {

using boost::filesystem::path;
using boost::filesystem::current_path;
using boost::filesystem::canonical;

namespace system { using boost::system::error_code; }

inline path canonical_and_preferred( const path& path_, const path& base = current_path() )
{ return canonical(path_, base).make_preferred(); }

inline path canonical_and_preferred( const path& path_, system::error_code& error_code )
{ return canonical(path_, error_code).make_preferred(); }

inline path canonical_and_preferred( const path& path_, const path& base, system::error_code& error_code )
{ return canonical(path_, base, error_code).make_preferred(); }

inline bool try_canonical_and_preferred( path& path_, const path& base = current_path() )
{ 
	system::error_code error_code;
	path_ = canonical(path_, base, error_code).make_preferred();
	return !error_code;
}



////////////////////////////////////////////////////////////////////////////////
/// Extension
///
/// Negative or zero "dots" value returns all extensions. E.g.:
/// extension("/usr/lib.cpp/file.vertex.shader.glsl.cache", -2) -> ".vertex.shader.glsl.cache"
/// extension("/usr/lib.cpp/file.vertex.shader.glsl.cache", -1) -> ".vertex.shader.glsl.cache"
/// extension("/usr/lib.cpp/file.vertex.shader.glsl.cache", 0)  -> ".vertex.shader.glsl.cache"
///
/// Positive "dots" value returns at most "dots" extensions (counting from the end of the path). E.g.:
/// extension("/usr/lib.cpp/file.vertex.shader.glsl.cache", 1)  -> ".cache"
/// extension("/usr/lib.cpp/file.vertex.shader.glsl.cache", 2)  -> ".glsl.cache"
/// extension("/usr/lib.cpp/file.vertex.shader.glsl.cache", 3)  -> ".shader.glsl.cache"
/// extension("/usr/lib.cpp/file.vertex.shader.glsl.cache", 4)  -> ".vertex.shader.glsl.cache"
/// extension("/usr/lib.cpp/file.vertex.shader.glsl.cache", 5)  -> ".vertex.shader.glsl.cache"
/// extension("/usr/lib.cpp/file.vertex.shader.glsl.cache", 6)  -> ".vertex.shader.glsl.cache"
///
/// Edge cases:
/// extension("var/", 0)  -> ""
/// extension("var/", 1)  -> ""
/// extension("var/", 2)  -> ""
/// extension("var/file", 0)  -> ""
/// extension("var/file", 1)  -> ""
/// extension("var/file", 2)  -> ""
/// extension("var/file.", 0)  -> "."
/// extension("var/file.cpp.", 0)  -> ".cpp."
/// extension("var/file.cpp...abc..", 0)  -> ".cpp...abc.."
/// extension("var/file.cpp...abc..", 1)  -> "."
/// extension("var/file.cpp...abc..", 2)  -> ".."
/// extension("var/file.cpp...abc..", 3)  -> ".abc.."
/// extension("var/file.cpp...abc..", 4)  -> "..abc.."
///
////////////////////////////////////////////////////////////////////////////////
inline path extension( const path& path_, int dots = 0 ) {
	// Get the filename to ensure that some edge cases are dealt with. E.g.:
	// path{"/var/foo.bar/baz.txt"}.filename() -> path{"baz.txt"}
	auto filename = path_.filename();
	const auto& native = filename.native();

	// Reverse search for the nth dot
	auto nth_dot = utility::find_last_or_nth(native.crbegin(), native.crend(), '.', dots).base();
	// Compensate for reverse_iterator -> iterator conversion
	if (native.cbegin() != nth_dot) --nth_dot;

	return {nth_dot, native.cend()};
}

} // namespace black_label

#endif
