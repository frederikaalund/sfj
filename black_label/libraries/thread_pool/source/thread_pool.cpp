#define BLACK_LABEL_THREAD_POOL_THREAD_POOL_IMPLEMENTATION
#include "black_label/thread_pool.hpp"
#include "black_label/shared_library/utility.hpp"



namespace black_label
{
namespace thread_pool
{

using boost::atomic;
using boost::unique_lock;
using boost::lock_guard;
using boost::mutex;
using boost::condition_variable;
using boost::thread_group;



thread_pool::thread_pool()
	: worker_count(boost::thread::hardware_concurrency())
	, workers(new worker*[worker_count])
	, worker_threads(new thread_group)
	, task_count(new atomic<int>(0))
	, waiting_for_workers(new mutex)
	, all_tasks_are_processed(new condition_variable)
{
	for (int i = 0; i < worker_count; ++i)
	{
		workers[i] = new worker(*this);
		boost::thread* thread = new boost::thread(boost::ref(*workers[i]));
		
		worker_threads->add_thread(thread);
	}
}

thread_pool::~thread_pool()
{
	worker_threads->interrupt_all();
	worker_threads->join_all();
	
	for (int i = 0; i < worker_count; ++i)
		delete workers[i];
	delete[] workers;
	delete worker_threads;
	delete task_count;
	delete waiting_for_workers;
	delete all_tasks_are_processed;
}

void thread_pool::processed_task( task& task )
{
	lock_guard<mutex> lock(*waiting_for_workers);

	if (0 == --(*task_count))
		all_tasks_are_processed->notify_all();
}



////////////////////////////////////////////////////////////////////////////////
/// Task Group
////////////////////////////////////////////////////////////////////////////////
thread_pool::task_group::task_group( thread_pool& pool )
	: pool(pool)
	, weight(0)
{
	 
}



void thread_pool::task_group::add( task& task )
{
	weight += task.weight;
	tasks.enqueue(task);
}

bool thread_pool::task_group::next( task& task )
{
	return tasks.dequeue(task);
}

void thread_pool::task_group::process( task task )
{
	weight -= task.weight;
	task();
	pool.processed_task(task);
}

void thread_pool::task_group::process_all()
{
	task task;
	while (tasks.dequeue(task))
		process(task);
}



////////////////////////////////////////////////////////////////////////////////
/// Worker
////////////////////////////////////////////////////////////////////////////////
thread_pool::worker::worker( thread_pool& pool )
	: tasks(pool)
{

}



void thread_pool::worker::add_task( task task )
{
	lock_guard<mutex> lock(processing_or_waiting);
	tasks.add(task);
	work_to_do.notify_all();
}

void thread_pool::worker::operator()()
{
	task task;
	unique_lock<mutex> lock(processing_or_waiting);

	while (true)
	{
		while (!tasks.next(task))
			work_to_do.wait(lock);

		lock.unlock();
		tasks.process(task);
		lock.lock();
	}
}



////////////////////////////////////////////////////////////////////////////////
/// Shared Library Runtime Interface
////////////////////////////////////////////////////////////////////////////////
BLACK_LABEL_SHARED_LIBRARY thread_pool* construct_thread_pool()
{
	return new thread_pool;
}

BLACK_LABEL_SHARED_LIBRARY void destroy_thread_pool( thread_pool const* pool )
{
	delete pool;
}

BLACK_LABEL_SHARED_LIBRARY void add_task( thread_pool* pool, task task )
{	
	(*pool->task_count)++;

	if (NOT_A_THREAD_ID != task.thread_affinity)
	{
		pool->workers[task.thread_affinity]->add_task(task);
	}

	
	
	static int thread = 0;

	pool->workers[thread]->add_task(task);
	if (pool->worker_count == ++thread) thread = 0;
}

BLACK_LABEL_SHARED_LIBRARY void add_raw_task(
	thread_pool* pool,
	task::function_type* task_function,
	task::data_type* task_data,
	thread_id_type thread_affinity,
	task::priority_type priority )
{	
	add_task(pool, task(task_function, task_data, thread_affinity, priority));
}

BLACK_LABEL_SHARED_LIBRARY void join( thread_pool* pool )
{
	unique_lock<mutex> lock(*pool->waiting_for_workers);

	while (0 < *pool->task_count)
		pool->all_tasks_are_processed->wait(lock);
}

} // namespace thread_pool
} // namespace black_label
