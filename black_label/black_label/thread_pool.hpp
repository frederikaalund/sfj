// Version 0.1
#ifndef BLACK_LABEL_THREAD_POOL_THREAD_POOL_HPP
#define BLACK_LABEL_THREAD_POOL_THREAD_POOL_HPP

#include "black_label/thread_pool/tasks.hpp"

#ifndef BLACK_LABEL_THREAD_POOL_DYNAMIC_INTERFACE
#include <boost/atomic.hpp>
#include <boost/lockfree/fifo.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#endif



namespace black_label
{
namespace thread_pool
{

class thread_pool
{
public:
	typedef black_label::thread_pool::tasks tasks_type;



////////////////////////////////////////////////////////////////////////////////
/// Static Interface
////////////////////////////////////////////////////////////////////////////////
#ifndef BLACK_LABEL_THREAD_POOL_DYNAMIC_INTERFACE

	thread_pool();
	~thread_pool();

	void schedule( tasks_type::size_type task );
	tasks_type::size_type create_and_schedule(
		tasks_type::function_type* function,
		tasks_type::data_type* data = 0,
		thread_id_type thread_affinity = NOT_A_THREAD_ID,
		tasks_type::weight_type weight = 0,
		tasks_type::size_type parent_count = 0,
		tasks_type::size_type* parents = 0 );
	void employ_current_thread();
	void join();
	void current_thread_id();

	tasks_type tasks;



private:
////////////////////////////////////////////////////////////////////////////////
/// Task Group
////////////////////////////////////////////////////////////////////////////////
	struct task_group
	{
		typedef boost::lockfree::fifo<tasks_type::size_type> task_queue_type;

		task_group( thread_pool& pool );

		void add( tasks_type::size_type task );
		bool next( tasks_type::size_type& task );
		void process( tasks_type::size_type task );

		thread_pool& pool;
		task_queue_type tasks;
		boost::atomic<tasks_type::weight_type> weight;
	};

	friend struct task_group;



////////////////////////////////////////////////////////////////////////////////
/// Worker
////////////////////////////////////////////////////////////////////////////////
	struct worker 
	{
		worker( thread_pool& pool );

		void add_task( tasks_type::size_type task );
		void start();
		void stop();
		void wake();

		void operator()();

		task_group tasks;
		boost::atomic<bool> 
			about_to_wait,
			work;
		boost::mutex waiting;
		boost::condition_variable work_to_do;
	};



	void stop_internal_workers();
	void stop_external_workers();
	void stop_workers();
	void wait_for_internal_workers_to_stop();
	void wait_for_external_workers_to_stop();
	void wait_for_workers_to_stop();
	void processed_task( tasks_type::size_type task );
	void add( tasks_type::size_type task );

	inline void schedule_children( tasks_type::size_type parent );

	int
		internal_worker_size,
		external_worker_capacity;
	worker
		** internal_workers,
		** external_workers;
	boost::thread_group
		worker_threads;
	boost::atomic<int>
		scheduled_task_size,
		external_worker_size,
		* remaining_parents_counts;
	boost::mutex
		waiting_for_workers;
	boost::condition_variable
		all_tasks_are_processed,
		all_external_workers_are_stopped;



////////////////////////////////////////////////////////////////////////////////
/// Dynamic Interface for Runtime-loaded Shared Libraries
////////////////////////////////////////////////////////////////////////////////
#else

public:
	struct method_pointers_type
	{
		void* (*construct)();
		void (*destroy)( void const* pool );
		void (*schedule)( 
			void* pool, 
			tasks_type::size_type task );
		tasks_type::size_type (*create_and_schedule)(
			void* pool,
			tasks_type::function_type* function,
			tasks_type::data_type* data,
			thread_id_type thread_affinity,
			tasks_type::weight_type weight,
			tasks_type::size_type parent_count,
			tasks_type::size_type* parents );
		void (*join)( void* pool );
		void (*employ_current_thread)( void* pool );
		void* (*tasks)( void* pool );
	} static method_pointers;

	thread_pool() 
		: _this(method_pointers.construct())
		, tasks(method_pointers.tasks(_this))
	{}
	~thread_pool() { method_pointers.destroy(_this); }
	void schedule( tasks_type::size_type task ) 
	{ method_pointers.schedule(_this, task); }
	tasks_type::size_type create_and_schedule( 
		tasks_type::function_type* function,
		tasks_type::data_type* data = 0,
		thread_id_type thread_affinity = NOT_A_THREAD_ID,
		tasks_type::weight_type weight = 0,
		tasks_type::size_type parent_count = 0,
		tasks_type::size_type* parents = 0 ) 
	{ 
		return method_pointers.create_and_schedule(
			_this,
			function,
			data,
			thread_affinity,
			weight,
			parent_count,
			parents);
	}
	void join()
	{ method_pointers.join(_this); }
	void employ_current_thread()
	{ method_pointers.employ_current_thread(_this); }

	void* _this;

	tasks_type tasks;

protected:
private:

#endif
};

} // namespace thread_pool
} // namespace black_label



#endif
