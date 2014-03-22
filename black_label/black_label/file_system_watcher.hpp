// Version 0.1
#ifndef BLACK_LABEL_FILE_SYSTEM_WATCHER_HPP
#define BLACK_LABEL_FILE_SYSTEM_WATCHER_HPP

#ifdef WINDOWS
#include <black_label/file_system_watcher/windows/data.hpp>
#endif

#include <black_label/file_system_watcher/types_and_constants.hpp>
#include <black_label/shared_library/utility.hpp>

#include <atomic>
#include <chrono>
#include <initializer_list>

#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>

#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_queue.h>



namespace tbb {

size_t tbb_hasher( const black_label::path& path_ );

} // namespace tbb



namespace black_label {
namespace file_system_watcher {

class BLACK_LABEL_SHARED_LIBRARY file_system_watcher 
#ifdef WINDOWS
	: private windows::data
#endif
{
public:
#ifdef WINDOWS
	using base_data = windows::data;
#endif
	using path_container = tbb::concurrent_queue<path>;
	using path_vector = std::vector<path>;
	using logged_path_container = tbb::concurrent_hash_map<path, std::chrono::steady_clock::time_point>;
	using threshold_type = std::chrono::seconds;

	friend void swap( file_system_watcher& lhs, file_system_watcher& rhs )
	{
		using std::swap;
		swap(static_cast<base_data&>(lhs), static_cast<base_data&>(rhs));
#ifdef WINDOWS
		for (auto path_watcher : lhs.path_watchers)
			path_watcher.second->fsw = &lhs;
		for (auto path_watcher : rhs.path_watchers)
			path_watcher.second->fsw = &rhs;
#endif
	}

	file_system_watcher() : last_cleanup{std::chrono::steady_clock::now()} {}
	file_system_watcher( path path, filter filter_  )
		: file_system_watcher{}
	{ subscribe(std::move(path), filter_); }
	file_system_watcher( 
		std::initializer_list<std::pair<path, filter>> entries )
		: file_system_watcher{}
	{ subscribe(std::move(entries)); }
	file_system_watcher( const file_system_watcher& other ) = delete;
	file_system_watcher( file_system_watcher&& rhs ) : file_system_watcher{} { swap(*this, rhs); }
	file_system_watcher& operator=( file_system_watcher rhs ) { swap(*this, rhs); return *this; }

	void subscribe( path path, filter filter_ );
	void unsubscribe( path path );

	void subscribe( std::initializer_list<std::pair<path, filter>> entries )
	{ for (auto entry : entries) subscribe(std::move(entry.first), entry.second); }
	void unsubscribe(std::initializer_list<path> paths)
	{ for (auto path : paths) unsubscribe(path); }

	path_vector get_unique_modified_paths()
	{
		using namespace std;

		// Allocate enough space to store all the paths.
		// Note that unsafe_size may return the incorrect size.
		// However, we are looking for an approximate anyhow.
		path_vector unique_modified_paths;
		unique_modified_paths.reserve(modified_paths.unsafe_size());

		// Get all modified paths
		for (path modified_path; modified_paths.try_pop(modified_path);)
			unique_modified_paths.push_back(modified_path);

		// Remove duplicate paths
		unique_modified_paths.resize(
			unique(begin(unique_modified_paths), end(unique_modified_paths))
			- begin(unique_modified_paths));

		return move(unique_modified_paths);
	}

	static bool is_expired( 
		std::chrono::steady_clock::time_point time_point,
		std::chrono::seconds threshold )
	{ return std::chrono::steady_clock::now() - time_point > threshold; }

	path_vector get_unique_modified_paths( std::chrono::seconds threshold )
	{
		using namespace std;
		using namespace std::chrono;

		boost::shared_lock<boost::shared_mutex> lock{logged_paths_mutex};

		path_vector unique_modified_paths;
		unique_modified_paths.reserve(modified_paths.unsafe_size());

		// Get all modified paths
		for (path modified_path; modified_paths.try_pop(modified_path);)
		{
			logged_path_container::accessor accessor;

			// Log the path
			if (logged_paths.insert(accessor, {modified_path, steady_clock::now()}))
				// Path was not logged before. The path is returned.
				unique_modified_paths.push_back(modified_path);
			// Path had an expired log entry
			else if (is_expired(accessor->second, threshold))
			{
				// The path is returned
				unique_modified_paths.push_back(modified_path);
				// The log entry is updated
				accessor->second = steady_clock::now();
			}
		}

		return move(unique_modified_paths);
	}

	void clear_logged_paths()
	{
		boost::lock_guard<boost::shared_mutex> lock{logged_paths_mutex};
		modified_paths.clear();
	}
	void clean_if_scheduled()
	{
		using namespace std::chrono;

		auto last_cleanup_ = last_cleanup.load();

		// It is not yet time to clean
		if (steady_clock::now() - last_cleanup_ <= seconds(5))
			return;

		// Make sure that only 1 thread actually performs the cleanup
		if (!last_cleanup.compare_exchange_strong(last_cleanup_, steady_clock::now()))
			return;

		// Clean
		clear_logged_paths();
	}
	void update()
	{
		clean_if_scheduled();
		update_internal();
	}



MSVC_PUSH_WARNINGS(4251)
	path_container modified_paths;
	logged_path_container logged_paths;
	boost::shared_mutex logged_paths_mutex;
	std::atomic<std::chrono::steady_clock::time_point> last_cleanup;
MSVC_POP_WARNINGS()



private:
	void update_internal();
};

} // namespace file_system_watcher
} // namespace black_label



#endif
