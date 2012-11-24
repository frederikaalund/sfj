#ifdef UNIX



#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/file_system_watcher.hpp>



namespace black_label {
namespace file_system_watcher {

////////////////////////////////////////////////////////////////////////////////
/// File System Watcher
////////////////////////////////////////////////////////////////////////////////
file_system_watcher::file_system_watcher( const path_type& path, const filters_type filters )
{
	add_path(path, filters);
}

file_system_watcher::~file_system_watcher()
{
}



void file_system_watcher::add_path( const path_type& path, const filters_type filters )
{
}

void file_system_watcher::update()
{
}

void file_system_watcher::release_resources()
{
}

} // namespace file_buffer
} // namespace black_label



#endif
