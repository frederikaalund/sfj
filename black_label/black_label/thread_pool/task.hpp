#ifndef BLACK_LABEL_THREAD_POOL_TASK_HPP
#define BLACK_LABEL_THREAD_POOL_TASK_HPP

#include <algorithm>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include <black_label/shared_library/utility.hpp>
#include <black_label/thread_pool/types.hpp>
#include <black_label/utility/boost_atomic_extensions.hpp>



namespace black_label
{
namespace thread_pool
{
  class task;
} // namespace thread_pool
} // namespace black_label

// The following operators and functions are declared in the global namespace
// to be compatible with clang 4.0. When declared in thread_pool::black_label,
// clang fails to implicitly convert lambdas into tasks to match the function
// signatures. In VS2010 this is not a problem. I don't know which compiler is
// working according to the standard.

BLACK_LABEL_SHARED_LIBRARY
black_label::thread_pool::task in_parallel( black_label::thread_pool::task&& lhs, black_label::thread_pool::task&& rhs );
BLACK_LABEL_SHARED_LIBRARY
black_label::thread_pool::task in_parallel( const black_label::thread_pool::task& lhs, const black_label::thread_pool::task& rhs );

BLACK_LABEL_SHARED_LIBRARY
black_label::thread_pool::task operator|( black_label::thread_pool::task&& lhs, black_label::thread_pool::task&& rhs );
BLACK_LABEL_SHARED_LIBRARY
black_label::thread_pool::task operator|( const black_label::thread_pool::task& lhs, const black_label::thread_pool::task& rhs );


BLACK_LABEL_SHARED_LIBRARY
black_label::thread_pool::task in_succession( black_label::thread_pool::task&& lhs, black_label::thread_pool::task&& rhs );
BLACK_LABEL_SHARED_LIBRARY
black_label::thread_pool::task in_succession( const black_label::thread_pool::task& lhs, const black_label::thread_pool::task& rhs );

BLACK_LABEL_SHARED_LIBRARY
black_label::thread_pool::task operator>>( const black_label::thread_pool::task& lhs, const black_label::thread_pool::task& rhs );
BLACK_LABEL_SHARED_LIBRARY
black_label::thread_pool::task operator>>( black_label::thread_pool::task&& lhs, black_label::thread_pool::task&& rhs );



namespace black_label
{
namespace thread_pool
{

////////////////////////////////////////////////////////////////////////////////
/// task
/// 
/// To be used with a thread_pool. Implicitly constructible from anything that
/// is callable. E.g.: lambdas, functors, and function pointers.
/// 
/// Some tasks are "group tasks". These tasks contain sub tasks to be processed
/// concurrently. Optionally, a successor task is scheduled once every sub task
/// has been processed.
////////////////////////////////////////////////////////////////////////////////
class BLACK_LABEL_SHARED_LIBRARY task
{
public:
	typedef int weight_type;

	friend class thread_pool;



	BLACK_LABEL_SHARED_LIBRARY friend void swap( task& lhs, task& rhs );

	task() : predecessor(nullptr), successor(nullptr), sub_tasks_left(0) {}

	template<typename F>
	task( F function )
    : function(function)
		, thread_affinity(NOT_A_THREAD_ID)
		, weight(1)
		, predecessor(nullptr)
		, successor(nullptr)
		, sub_tasks_left(0)
	{}

	task( task&& other ) : predecessor(nullptr), successor(nullptr) 
	{ swap(*this, other); }

	task( const task& other );

	~task();

	task& operator=( task other ) { swap(*this, other); return *this; }
	void operator()() { function(); }



private:
	task( const task* other, task* predecessor )
		: function(other->function)
		, thread_affinity(other->thread_affinity)
		, weight(other->weight)
		, predecessor(predecessor)
		, sub_tasks(other->sub_tasks)
	{}
	


MSVC_PUSH_WARNINGS(4251)
	static task* DUMMY_TASK;



	bool is_group_task() const { return !sub_tasks.empty(); }
	bool is_root() const { return !predecessor || DUMMY_TASK == predecessor; }
	bool is_owned_by_thread_pool() const { return DUMMY_TASK == predecessor; }

	void give_ownership_to_thread_pool() { predecessor = DUMMY_TASK; }
	void restore_task_hierarchy();
	void reset_task_counts();
	task& last_successor();

	

	// Single task properties
	std::function<void()> function;
	thread_id_type thread_affinity;
	weight_type weight;
	task* predecessor;

	// Group task properties
	std::vector<task> sub_tasks; // Processed concurrently.
	task* successor; // Processed after all sub_tasks are processed.
	boost::atomic<int> sub_tasks_left;
MSVC_POP_WARNINGS()



public:
	task& in_parallel( task&& rhs );
	task& in_parallel( const task& rhs );

	task& operator|=( task&& rhs );
	task& operator|=( const task& rhs );


	task& in_succession( task&& rhs );
	task& in_succession( const task& rhs );

	task& operator>>=( task&& rhs );
	task& operator>>=( const task& rhs );
  
  
	BLACK_LABEL_SHARED_LIBRARY
	friend task (::in_parallel)( task&& lhs, task&& rhs );
	BLACK_LABEL_SHARED_LIBRARY
	friend task (::in_parallel)( const task& lhs, const task& rhs );
  
	BLACK_LABEL_SHARED_LIBRARY
	friend task (::operator|)( task&& lhs, task&& rhs );
	BLACK_LABEL_SHARED_LIBRARY
	friend task (::operator|)( const task& lhs, const task& rhs );
  
  
	BLACK_LABEL_SHARED_LIBRARY
	friend task (::in_succession)( task&& lhs, task&& rhs );
	BLACK_LABEL_SHARED_LIBRARY
	friend task (::in_succession)( const task& lhs, const task& rhs );
  
	BLACK_LABEL_SHARED_LIBRARY
	friend task (::operator>>)( const task& lhs, const task& rhs );
	BLACK_LABEL_SHARED_LIBRARY
	friend task (::operator>>)( task&& lhs, task&& rhs );
};

} // namespace thread_pool
} // namespace black_label



#endif
