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
class task
{
public:
	typedef int weight_type;

	friend void swap( task& lhs, task& rhs )
	{
		using std::swap;
		swap(lhs.function, rhs.function);
		swap(lhs.thread_affinity, rhs.thread_affinity);
		swap(lhs.weight, rhs.weight);
		swap(lhs.owner, rhs.owner);
		swap(lhs.sub_tasks, rhs.sub_tasks);
		swap(lhs.successor, rhs.successor);
		swap(lhs.sub_tasks_left, rhs.sub_tasks_left);
	}

	task() : owner(nullptr), successor(nullptr), sub_tasks_left(0) {}

	template<typename F>
	task( const F& function ) 
		: function(function)
		, thread_affinity(NOT_A_THREAD_ID)
		, weight(1)
		, owner(nullptr)
		, successor(nullptr)
		, sub_tasks_left(0)
	{}

	task ( task&& other ) { swap(*this, other); }

	task( const task& other )
		: function(other.function)
		, thread_affinity(other.thread_affinity)
		, weight(other.weight)
		, owner(other.owner)
		, sub_tasks(other.sub_tasks)
		, successor((other.successor) ? new task(*other.successor) : nullptr)
		, sub_tasks_left(other.sub_tasks_left.load())
	{}

	task& operator=( task other ) { swap(*this, other); return *this; }
	void operator()() { function(); }

	bool is_group_task() const { return !sub_tasks.empty(); }

	void register_sub_tasks()
	{
		std::for_each(sub_tasks.begin(), sub_tasks.end(), 
			[this]( task& sub_task )
		{
			sub_task.owner = this;
			sub_task.register_sub_tasks();
		});

		if (successor)
		{
			successor->owner = this;
			successor->register_sub_tasks();
		}

		sub_tasks_left.store(sub_tasks.size());
	}



	BLACK_LABEL_SHARED_LIBRARY friend task operator|( task&& lhs, task&& rhs );
	BLACK_LABEL_SHARED_LIBRARY friend task operator>>( task&& lhs, task&& rhs );

	BLACK_LABEL_SHARED_LIBRARY friend task operator|( const task& lhs, const task& rhs );
	BLACK_LABEL_SHARED_LIBRARY friend task operator>>( const task& lhs, const task& rhs );

	BLACK_LABEL_SHARED_LIBRARY friend task& operator|=( task& lhs, task&& rhs );
	BLACK_LABEL_SHARED_LIBRARY friend task& operator>>=( task& lhs, task&& rhs );



	// Single task properties
	std::function<void()> function;
	thread_id_type thread_affinity;
	weight_type weight;

	task* owner; // Owning group task

	// Group task properties
	std::vector<task> sub_tasks; // Processed concurrently.
	std::unique_ptr<task> successor; // Processed after all sub_tasks are processed.
	boost::atomic<int> sub_tasks_left;

protected:
	task& last_successor()
	{
		// Find sub_task's successor chain's end
		task* result = this;
		while (result->successor)
			result = result->successor.get();
		return *result;
	}
private:
};

} // namespace thread_pool
} // namespace black_label



#endif
