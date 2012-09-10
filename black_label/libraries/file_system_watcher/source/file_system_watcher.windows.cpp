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



namespace black_label
{
namespace file_system_watcher
{

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
	path_watchers.push_back(std::unique_ptr<path_watcher>(new path_watcher(this, path, filters)));
}

void file_system_watcher::update()
{
	SleepEx(0, TRUE);
	modified_paths.resize(
		std::unique(modified_paths.begin(), modified_paths.end())
		- modified_paths.begin());
}

void file_system_watcher::release_resources()
{
	std::for_each(path_watchers.begin(), path_watchers.end(), 
		[] ( std::unique_ptr<path_watcher>& pw ) { pw->release_resources(); });
}



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
	const path_type& path,
	const filters_type filters )
	: fsw(fsw)
	, filters(filters)
{
	if (INVALID_HANDLE_VALUE == (directory_or_file_handle = CreateFile(
		path.c_str(),
		FILE_LIST_DIRECTORY,
		FILE_SHARE_READ	| FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
		NULL)))
		throw std::system_error(
			GetLastError(), 
			std::system_category(), 
			error_message(last));

	buffer = _aligned_malloc(buffer_size, sizeof(DWORD));

	std::fill(
		reinterpret_cast<char*>(&over), 
		reinterpret_cast<char*>(&over) + sizeof(OVERLAPPED), 
		0);
	over.hEvent = static_cast<HANDLE>(this);

	win_filters = 0;
	if (FILTER_WRITE == filters) win_filters |= FILE_NOTIFY_CHANGE_LAST_WRITE;

	read_changes();
}

file_system_watcher::path_watcher::~path_watcher()
{
	try { release_resources(); } catch(const std::exception&) {}
}



void file_system_watcher::path_watcher::read_changes()
{
	DWORD bytes_returned;
	if (!ReadDirectoryChangesW(
		directory_or_file_handle,
		buffer,
		buffer_size,
		FALSE,
		win_filters,
		&bytes_returned,
		&over,
		&completion))
		throw std::system_error(
			GetLastError(), 
			std::system_category(), 
			error_message(last));

	if (0 == bytes_returned)
		throw std::exception("Buffer overflowed.");
}

void file_system_watcher::path_watcher::release_resources()
{
	if (INVALID_HANDLE_VALUE == directory_or_file_handle) return;

	_aligned_free(buffer);

	if (!CancelIoEx(directory_or_file_handle, NULL))
		throw std::system_error(
			GetLastError(), 
			std::system_category(), 
			error_message(last));

	if (!CloseHandle(directory_or_file_handle))
		throw std::system_error(
			GetLastError(), 
			std::system_category(), 
			error_message(last));

	directory_or_file_handle = INVALID_HANDLE_VALUE;
}



VOID CALLBACK file_system_watcher::path_watcher::completion( 
	DWORD dwErrorCode, 
	DWORD dwNumberOfBytesTransfered, 
	LPOVERLAPPED lpOverlapped )
{
	if (dwErrorCode || 0 == dwNumberOfBytesTransfered) return;

	path_watcher* pw = static_cast<path_watcher*>(lpOverlapped->hEvent);

	FILE_NOTIFY_INFORMATION* fni = static_cast<FILE_NOTIFY_INFORMATION*>(pw->buffer);
	do 
	{
		static_assert(sizeof(wchar_t) == sizeof(WCHAR), "size of wchar_t does not equal size of WCHAR");

		int size = WideCharToMultiByte(CP_UTF8, 0, fni->FileName, fni->FileNameLength / sizeof(WCHAR), NULL, 0, 0, 0);
		std::unique_ptr<char[]> string_buffer(new char[size]);
		WideCharToMultiByte(CP_UTF8, 0, fni->FileName, fni->FileNameLength / sizeof(WCHAR), string_buffer.get(), size, 0, 0);
		pw->fsw->modified_paths.push_back(path_type(string_buffer.get(), size));

		fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<char*>(fni) + fni->NextEntryOffset);
	} while (0 < fni->NextEntryOffset);

	pw->read_changes();
}

} // namespace file_buffer
} // namespace black_label



#endif
