// Version 0.1
#ifndef BLACK_LABEL_THREAD_POOL_THREAD_POOL_HPP
#define BLACK_LABEL_THREAD_POOL_THREAD_POOL_HPP

#include "black_label/thread_pool/task.hpp"

#ifdef BLACK_LABEL_THREAD_POOL_THREAD_POOL_IMPLEMENTATION
#include <boost/atomic.hpp>
#include <boost/lockfree/ringbuffer.hpp>
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
	thread_pool();
	~thread_pool();

	int worker_count;
	thread_id_type main_thread_id;
protected:
private:
	void processed_task( task& task );



////////////////////////////////////////////////////////////////////////////////
/// Implementation Specific
////////////////////////////////////////////////////////////////////////////////
#ifdef BLACK_LABEL_THREAD_POOL_THREAD_POOL_IMPLEMENTATION



////////////////////////////////////////////////////////////////////////////////
/// Task Group
////////////////////////////////////////////////////////////////////////////////
	struct task_group
	{
		typedef boost::lockfree::ringbuffer<task, 16384> task_queue_type;

		task_group( thread_pool& pool );

		void add( task& task );
		bool next( task& task );
		void process( task task );

		void process_all();

		thread_pool& pool;
		task_queue_type tasks;
		boost::atomic<task::weight_type> weight;
	};

	friend struct task_group;



////////////////////////////////////////////////////////////////////////////////
/// Worker
////////////////////////////////////////////////////////////////////////////////
	struct worker 
	{
		worker( thread_pool& pool );

		void add_task( task task );

		void operator()();

		task_group tasks;
		boost::mutex processing_or_waiting;
		boost::condition_variable work_to_do;
	};



public:
	worker** workers;
	boost::thread_group* worker_threads;
	boost::atomic<int>* task_count;
	boost::mutex* waiting_for_workers;
	boost::condition_variable* all_tasks_are_processed;
#else
	void* ignore[6]; // Make sure the overall size is identical.
#endif



////////////////////////////////////////////////////////////////////////////////
/// Shared Library Runtime Interface
////////////////////////////////////////////////////////////////////////////////
public:
	typedef thread_pool* construct_thread_pool_type();
	typedef void destroy_thread_pool_type( thread_pool const* pool );
	typedef void add_task_type( thread_pool* pool, task task );
	typedef void add_raw_task_type(
		thread_pool* pool,
		task::function_type* task_function,
		task::data_type* task_data,
		thread_id_type thread_affinity,
		task::priority_type priority );
	typedef void join_type( thread_pool* pool );

	static construct_thread_pool_type* construct;
	static destroy_thread_pool_type* destroy;
	static add_task_type* add_task;
	static add_raw_task_type* add_raw_task;
	static join_type* join;
protected:
private:
};

} // namespace thread_pool
} // namespace black_label



#endif
