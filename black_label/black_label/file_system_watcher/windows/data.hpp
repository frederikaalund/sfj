// Version 0.1
#ifndef BLACK_LABEL_FILE_SYSTEM_WATCHER_WINDOWS_DATA_HPP
#define BLACK_LABEL_FILE_SYSTEM_WATCHER_WINDOWS_DATA_HPP

#include <black_label/file_system_watcher/types_and_constants.hpp>
#include <black_label/shared_library/utility.hpp>

#include <memory>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>



namespace black_label {
namespace file_system_watcher {

class file_system_watcher;

namespace windows {

class BLACK_LABEL_SHARED_LIBRARY data
{
public:
	data() {}

	class BLACK_LABEL_SHARED_LIBRARY path_watcher
	{
	public:
		const static size_t buffer_size = 1024 * 64; // 64 KiB

		path_watcher() : directory_or_file_handle(INVALID_HANDLE_VALUE) {}
		path_watcher( 
			file_system_watcher* fsw, 
			const path_type& path, 
			const filters_type filters );
		~path_watcher();

		void read_changes();
		void release_resources();

		static VOID CALLBACK completion(
			DWORD dwErrorCode,
			DWORD dwNumberOfBytesTransfered,
			LPOVERLAPPED lpOverlapped);

	protected:
		path_watcher( const path_watcher& other );
		path_watcher& operator=( path_watcher rhs );

	private:
		file_system_watcher* fsw;
		filters_type filters;
		DWORD win_filters;
		HANDLE directory_or_file_handle;
		void* buffer;
		OVERLAPPED over;
	};



#pragma warning(push)
#pragma warning(disable : 4251)

	std::vector<std::unique_ptr<path_watcher>> path_watchers;

#pragma warning(pop)



protected:
	data( const data& other );
	data& operator=( data rhs );
};

} // namespace windows
} // namespace file_system_watcher
} // namespace black_label



#endif
