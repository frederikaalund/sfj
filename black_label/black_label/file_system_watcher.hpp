// Version 0.1
#ifndef BLACK_LABEL_FILE_SYSTEM_WATCHER_HPP
#define BLACK_LABEL_FILE_SYSTEM_WATCHER_HPP

#ifdef WINDOWS
#include <black_label/file_system_watcher/windows/data.hpp>
#endif

#include <black_label/file_system_watcher/types_and_constants.hpp>
#include <black_label/shared_library/utility.hpp>

#include <vector>



namespace black_label {
namespace file_system_watcher {

class BLACK_LABEL_SHARED_LIBRARY file_system_watcher 
#ifdef WINDOWS
	: private windows::data
#endif
{
public:
	typedef std::vector<path_type> path_container_type;

	file_system_watcher() {}
	file_system_watcher( const path_type& path, const filters_type filters );
	~file_system_watcher();

	// These calls are not thread-safe
	void add_path( const path_type& path, const filters_type filters );
	void update();

	// Releases all system resources. Subsequent method calls to 
	// file_system_watcher are undefined except for the destructor.
	void release_resources();

	// This member is not thread-safe  
MSVC_PUSH_WARNINGS(4251)
	path_container_type modified_paths;
MSVC_POP_WARNINGS()



protected:
	file_system_watcher( const file_system_watcher& other );
	file_system_watcher& operator=( file_system_watcher rhs );

private:
};

} // namespace file_system_watcher
} // namespace black_label



#endif
