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
	: tasks(50000)
	, internal_worker_size(boost::thread::hardware_concurrency()-1)
	, external_worker_capacity(1)
	, internal_workers(new worker*[internal_worker_size])
	, external_workers(new worker*[external_worker_capacity])
	, scheduled_task_size(0)
	, external_worker_size(0)
	, remaining_parents_counts(new atomic<int>[tasks.capacity])
{
	for (int i = 0; i < internal_worker_size; ++i)
	{
		internal_workers[i] = new worker(*this);
		boost::thread* thread = new boost::thread(boost::ref(*internal_workers[i]));
		
		worker_threads.add_thread(thread);
	}

	for (int i = 0; i < external_worker_capacity; ++i)
	{
		external_workers[i] = new worker(*this);
	}
}

thread_pool::~thread_pool()
{
	stop_workers();
	wait_for_workers_to_stop();

	for (int i = 0; i < internal_worker_size; ++i)
		delete internal_workers[i];
	delete[] internal_workers;
	for (int i = 0; i < external_worker_capacity; ++i)
		delete external_workers[i];
	delete[] external_workers;
	delete[] remaining_parents_counts;
}

void thread_pool::schedule( tasks_type::size_type task )
{
	scheduled_task_size++;

	remaining_parents_counts[task].store(tasks.parent_counts[task]);

	if (tasks.has_parents(task)) return;

	add(task);
}

thread_pool::tasks_type::size_type thread_pool::create_and_schedule( 
	tasks_type::function_type* function, 
	tasks_type::data_type* data, 
	thread_id_type thread_affinity, 
	tasks_type::weight_type weight, 
	tasks_type::size_type parent_count, 
	tasks_type::size_type* parents )
{
	tasks_type::size_type task = tasks.create(
		function, 
		data, 
		thread_affinity, 
		weight, 
		parent_count, 
		parents);
	schedule(task);
	return task;
}

void thread_pool::employ_current_thread()
{
	int id;

	{
		lock_guard<mutex> lock(waiting_for_workers);
		id = external_worker_size++;
	}

	external_workers[id]->start();

	{
		lock_guard<mutex> lock(waiting_for_workers);
		if (0 == external_worker_size--)
			all_external_workers_are_stopped.notify_all();
	}
}

void thread_pool::join()
{
	unique_lock<mutex> lock(waiting_for_workers);

	while (0 < scheduled_task_size)
		all_tasks_are_processed.wait(lock);
}

void thread_pool::current_thread_id()
{
	boost::thread::id id = boost::this_thread::get_id();
	
}



void thread_pool::stop_internal_workers()
{
	for (int i = 0; i < internal_worker_size; ++i)
		internal_workers[i]->stop();
}
void thread_pool::stop_external_workers()
{
	for (int i = 0; i < external_worker_size; ++i)
		external_workers[i]->stop();
}
void thread_pool::stop_workers()
{
	stop_internal_workers();
	stop_external_workers();
}

void thread_pool::wait_for_internal_workers_to_stop()
{
	worker_threads.join_all();
}
void thread_pool::wait_for_external_workers_to_stop()
{
	unique_lock<mutex> lock(waiting_for_workers);
	while (0 < external_worker_size)
		all_external_workers_are_stopped.wait(lock);
}
void thread_pool::wait_for_workers_to_stop()
{
	wait_for_internal_workers_to_stop();
	wait_for_external_workers_to_stop();
}

void thread_pool::processed_task( tasks_type::size_type task )
{
	schedule_children(task);

	if (0 < --scheduled_task_size) return;

	stop_external_workers();

	lock_guard<mutex> lock(waiting_for_workers);
	all_tasks_are_processed.notify_all();
}

void thread_pool::add( tasks_type::size_type task )
{
	if (NOT_A_THREAD_ID != tasks.thread_affinities[task])
	{
		internal_workers[tasks.thread_affinities[task]]->add_task(task);
		return;
	}



	worker* least_burdened_worker = internal_workers[0];
	for (int i = 1; i < internal_worker_size; ++i)
	{
		worker* potential = internal_workers[i];
		if (least_burdened_worker->tasks.weight > potential->tasks.weight)
			least_burdened_worker = potential;
	}
	for (int i = 0; i < external_worker_size; ++i)
	{
		worker* potential = external_workers[i];
		if (least_burdened_worker->tasks.weight > potential->tasks.weight)
			least_burdened_worker = potential;
	}

	least_burdened_worker->add_task(task);
}



void thread_pool::schedule_children( tasks_type::size_type parent )
{
	for (tasks_type::size_type i = 0; i < tasks.child_counts[parent]; ++i)
	{
		tasks_type::size_type child = 
			tasks.children[tasks.max_children * parent + i];
		if (0 == --remaining_parents_counts[child])
			add(child);
	}
}



////////////////////////////////////////////////////////////////////////////////
/// Task Group
////////////////////////////////////////////////////////////////////////////////
thread_pool::task_group::task_group( thread_pool& pool )
	: pool(pool)
	, weight(0)
{
	 
}



void thread_pool::task_group::add( tasks_type::size_type task )
{
	weight += pool.tasks.weights[task];
	tasks.enqueue(task);
}

bool thread_pool::task_group::next( tasks_type::size_type& task )
{
	return tasks.dequeue(task);
}

void thread_pool::task_group::process( tasks_type::size_type task )
{
	pool.tasks.run(task);
	weight -= pool.tasks.weights[task];
	pool.processed_task(task);
}



////////////////////////////////////////////////////////////////////////////////
/// Worker
////////////////////////////////////////////////////////////////////////////////
thread_pool::worker::worker( thread_pool& pool )
	: tasks(pool)
	, about_to_wait(false)
	, work(true)
{

}



void thread_pool::worker::start()
{
	work = true;
	(*this)();
}

void thread_pool::worker::stop()
{
	work = false;
	wake();
}

void thread_pool::worker::wake()
{
	if (!about_to_wait) return;

	lock_guard<mutex> lock(waiting);
	work_to_do.notify_one();
}

void thread_pool::worker::add_task( tasks_type::size_type task )
{
	tasks.add(task);
	wake();
}

void thread_pool::worker::operator()()
{
	tasks_type::size_type task;

	while (true)
	{
		// Test-test-and-wait
		if (!tasks.next(task) && work)
		{
			about_to_wait = true;
			unique_lock<mutex> lock(waiting);
			while (!tasks.next(task) && work)
				work_to_do.wait(lock);
			about_to_wait = false;
		}
		if (!work) return;

		tasks.process(task);
	}
}



////////////////////////////////////////////////////////////////////////////////
/// Dynamic Interface for Runtime-loaded Shared Libraries
////////////////////////////////////////////////////////////////////////////////
BLACK_LABEL_SHARED_LIBRARY thread_pool* thread_pool_construct()
{
	return new thread_pool();
}

BLACK_LABEL_SHARED_LIBRARY void thread_pool_destroy( thread_pool const* pool )
{
	delete pool;
}

BLACK_LABEL_SHARED_LIBRARY void thread_pool_schedule( 
	thread_pool* pool, 
	thread_pool::tasks_type::size_type task )
{	
	pool->schedule(task);
}

BLACK_LABEL_SHARED_LIBRARY thread_pool::tasks_type::size_type 
thread_pool_create_and_schedule( 
	thread_pool* pool,
	thread_pool::tasks_type::function_type* function,
	thread_pool::tasks_type::data_type* data,
	thread_id_type thread_affinity,
	thread_pool::tasks_type::weight_type weight,
	thread_pool::tasks_type::size_type parent_count,
	thread_pool::tasks_type::size_type* parents ) 
{
	return pool->create_and_schedule(
		function,
		data,
		thread_affinity,
		weight,
		parent_count,
		parents);
}

BLACK_LABEL_SHARED_LIBRARY void thread_pool_employ_current_thread( thread_pool* pool )
{
	pool->employ_current_thread();
}

BLACK_LABEL_SHARED_LIBRARY void thread_pool_join( thread_pool* pool )
{
	pool->join();
}

BLACK_LABEL_SHARED_LIBRARY thread_pool::tasks_type* thread_pool_tasks( thread_pool* pool )
{
	return &pool->tasks;
}

} // namespace thread_pool
} // namespace black_label
