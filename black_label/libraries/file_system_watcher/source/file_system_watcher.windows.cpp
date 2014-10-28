#ifdef WINDOWS



#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/file_system_watcher.hpp>

#include <algorithm>
#include <cassert>
#include <memory>
#include <system_error>
#include <type_traits>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>



namespace black_label {
namespace file_system_watcher {

////////////////////////////////////////////////////////////////////////////////
/// File System Watcher
////////////////////////////////////////////////////////////////////////////////
void file_system_watcher::subscribe( path path, const filter filter_ )
{
	path_watchers.insert({
		path.make_preferred(), 
		std::make_shared<path_watcher>(this, path, filter_)});
}

void file_system_watcher::unsubscribe( path path )
{ path_watchers.erase(path.make_preferred()); }

void file_system_watcher::update_internal()
{ SleepEx(0, TRUE); }



////////////////////////////////////////////////////////////////////////////////
/// Error Message
////////////////////////////////////////////////////////////////////////////////
struct last_type {};
const last_type last;

struct error_message
{
	error_message( last_type )
	{
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER
			| FORMAT_MESSAGE_FROM_SYSTEM
			| FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, 
			GetLastError(), 
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			reinterpret_cast<LPTSTR>(&buffer),
			0,
			NULL);
	}
	~error_message() { LocalFree(buffer); }

	static_assert(sizeof(char) == sizeof(TCHAR), "Size of char does not equal size of TCHAR");
	operator char*() { return reinterpret_cast<char*>(buffer); }

	LPVOID buffer;
};



////////////////////////////////////////////////////////////////////////////////
/// Path Watcher
////////////////////////////////////////////////////////////////////////////////
file_system_watcher::path_watcher::path_watcher( 
	file_system_watcher* fsw,
	const path& path_,
	const filter filter_ )
	: fsw{fsw}
	, filter_{filter_}
	, win_filters{0}
{
	if (INVALID_HANDLE_VALUE == (directory_or_file_handle = CreateFile(
		path_.string().c_str(),
		FILE_LIST_DIRECTORY,
		FILE_SHARE_READ	| FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
		NULL)))
		throw std::system_error(
			GetLastError(), 
			std::system_category(), 
			error_message{last});

	buffer = _aligned_malloc(buffer_size, sizeof(DWORD));

	std::fill_n(reinterpret_cast<char*>(&over), sizeof(OVERLAPPED), 0);
	over.hEvent = static_cast<HANDLE>(this);

	if (filter::write & filter_) win_filters |= FILE_NOTIFY_CHANGE_LAST_WRITE;
	if (filter::access & filter_) win_filters |= FILE_NOTIFY_CHANGE_LAST_ACCESS;
	if (filter::file_name & filter_) win_filters |= FILE_NOTIFY_CHANGE_FILE_NAME;

	read_changes();
}

file_system_watcher::path_watcher::~path_watcher()
{ try { release_resources(); } catch( const std::exception& exception ) { assert(false); } }



void file_system_watcher::path_watcher::read_changes()
{
	DWORD bytes_returned;
	if (!ReadDirectoryChangesW(
		directory_or_file_handle,
		buffer,
		buffer_size,
		TRUE,
		win_filters,
		&bytes_returned,
		&over,
		&completion))
		throw std::system_error(
			GetLastError(), 
			std::system_category(), 
			error_message{last});
}

void file_system_watcher::path_watcher::release_resources()
{
	// No resources were allocated
	if (INVALID_HANDLE_VALUE == directory_or_file_handle) return;

	// Terminate the completion routine
	if (!CancelIo(directory_or_file_handle))
		throw std::system_error{
			static_cast<int>(GetLastError()),
			std::system_category(), 
			error_message{last}};

	// Wait for termination
	while (!terminated) { SleepEx(INFINITE, TRUE); }

	// Free resources
	_aligned_free(buffer);
	if (!CloseHandle(directory_or_file_handle))
		throw std::system_error{ 
			static_cast<int>(GetLastError()), 
			std::system_category(), 
			error_message{last}};

	// Flag that resources have been freed
	directory_or_file_handle = INVALID_HANDLE_VALUE;
}



VOID CALLBACK file_system_watcher::path_watcher::completion( 
	DWORD dwErrorCode, 
	DWORD dwNumberOfBytesTransfered, 
	LPOVERLAPPED lpOverlapped )
{
	// Retrieve the path_watcher pointer (essentially making this function pseudo-non-static)
	auto pw = static_cast<path_watcher*>(lpOverlapped->hEvent);

	// The watch is over...
	if (ERROR_OPERATION_ABORTED == dwErrorCode) {
		// ...so signal that the completion routine has been called the last time.
		pw->terminated = true;
		return;
	}

	// Any error (including buffer overflows) are silently ignored
	if (dwErrorCode || 0 == dwNumberOfBytesTransfered) return;

	// Process the linked list of notifications
	auto fni = static_cast<FILE_NOTIFY_INFORMATION*>(pw->buffer);
	do 
	{
		static_assert(sizeof(wchar_t) == sizeof(WCHAR), "size of wchar_t does not equal size of WCHAR");

		auto size = WideCharToMultiByte(CP_UTF8, 0, fni->FileName, fni->FileNameLength / sizeof(WCHAR), NULL, 0, 0, 0);
		auto string_buffer = std::make_unique<char[]>(size);
		WideCharToMultiByte(CP_UTF8, 0, fni->FileName, fni->FileNameLength / sizeof(WCHAR), string_buffer.get(), size, 0, 0);
		pw->fsw->modified_paths.push(path(string_buffer.get(), &string_buffer[size]));

		fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<char*>(fni) + fni->NextEntryOffset);
	} while (0 < fni->NextEntryOffset);

	// Reissue the watcher
	pw->read_changes();
}



} // namespace file_buffer
} // namespace black_label



#endif
