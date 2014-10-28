// Version 0.1
#ifndef BLACK_LABEL_FILE_SYSTEM_WATCHER_WINDOWS_DATA_HPP
#define BLACK_LABEL_FILE_SYSTEM_WATCHER_WINDOWS_DATA_HPP

#include <black_label/file_system_watcher/types_and_constants.hpp>
#include <black_label/shared_library/utility.hpp>

#include <atomic>
#include <memory>
#include <utility>

#include <tbb/concurrent_hash_map.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>



namespace black_label {
namespace file_system_watcher {

class file_system_watcher;

namespace windows {

class BLACK_LABEL_SHARED_LIBRARY data
{
public:

	class BLACK_LABEL_SHARED_LIBRARY path_watcher
	{
	public:
		const static size_t buffer_size{1024 * 640}; // 640 KiB

		friend void swap( path_watcher& lhs, path_watcher& rhs )
		{
			using std::swap;
			swap(lhs.fsw, rhs.fsw);
			swap(lhs.filter_, rhs.filter_);
			swap(lhs.win_filters, rhs.win_filters);
			swap(lhs.directory_or_file_handle, rhs.directory_or_file_handle);
			swap(lhs.buffer, rhs.buffer);
			swap(lhs.over, rhs.over);
		}

		path_watcher() : directory_or_file_handle{INVALID_HANDLE_VALUE} {}
		path_watcher( 
			file_system_watcher* fsw, 
			const path& path_, 
			const filter filter_ );
		path_watcher( const path_watcher& other ) = delete;
		path_watcher( path_watcher&& other )
			: path_watcher{} { swap(*this, other); }
		~path_watcher();
		path_watcher& operator=( path_watcher rhs )
		{ swap(*this, rhs); return *this; }

		void read_changes();
		void release_resources();

		static VOID CALLBACK completion(
			DWORD dwErrorCode,
			DWORD dwNumberOfBytesTransfered,
			LPOVERLAPPED lpOverlapped);



		file_system_watcher* fsw;
		filter filter_;
		DWORD win_filters;
		HANDLE directory_or_file_handle;
		void* buffer;
		OVERLAPPED over;

		std::atomic<bool> terminated{false};
	};



	friend void swap( data& lhs, data& rhs )
	{
		using std::swap;
		swap(lhs.path_watchers, rhs.path_watchers);
	}



#pragma warning(push)
#pragma warning(disable : 4251)

	tbb::concurrent_hash_map<path, std::shared_ptr<path_watcher>> path_watchers;

#pragma warning(pop)
};

} // namespace windows
} // namespace file_system_watcher
} // namespace black_label



#endif
