#ifdef UNIX



#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/file_system_watcher.hpp>



namespace black_label {
namespace file_system_watcher {

////////////////////////////////////////////////////////////////////////////////
/// File System Watcher
////////////////////////////////////////////////////////////////////////////////
file_system_watcher::file_system_watcher( const path& path, const filters filters )
{
	subscribe(path, filters);
}

file_system_watcher::~file_system_watcher()
{
}



void file_system_watcher::subscribe( const path& path, const filters filters )
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
