#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/thread_pool.hpp>

#include <black_label/utility/algorithm.hpp>



namespace black_label
{
namespace thread_pool
{



using std::for_each;
using utility::min_element;

using boost::atomic;
using boost::unique_lock;
using boost::lock_guard;
using boost::mutex;
using boost::condition_variable;
using boost::thread_group;



thread_pool::thread_pool()
	: workers(boost::thread::hardware_concurrency())
	, scheduled_task_count(0)
{
	for_each(workers.begin(), workers.end(), [this] ( worker& worker ) {
		worker.pool = this;
		boost::thread* thread = new boost::thread(boost::ref(worker));
		worker_threads.add_thread(thread);
	});
}

thread_pool::~thread_pool()
{
	join();
	stop_workers();
	wait_for_workers_to_stop();
}



void thread_pool::schedule( const task& task )
{
	// Make a copy for internal use.
	typedef black_label::thread_pool::task task_type;
	task_type* task_copy = new task_type(task);
	task_copy->give_ownership_to_thread_pool();
	task_copy->restore_task_hierarchy();

	digest_task(task_copy);
}
void thread_pool::schedule( task* task )
{
	task->restore_task_hierarchy();
	digest_task(task);
}

void thread_pool::join()
{
	unique_lock<mutex> lock(waiting_for_workers);

	while (0 < scheduled_task_count)
		all_tasks_are_processed.wait(lock);
}

void thread_pool::current_thread_id()
{
	//boost::thread::id id = boost::this_thread::get_id();
}



void thread_pool::digest_task( task* task )
{
	typedef black_label::thread_pool::task task_type;

	// Distribute sub tasks.
	if (task->is_group_task())
		for_each(task->sub_tasks.begin(), task->sub_tasks.end(),
			[this] ( task_type& sub_task ) { digest_task(&sub_task); });
	// Schedule task.
	else if (task->function)
	{
		scheduled_task_count++;
		add(task);
	}
	// Task was empty - clean up after it.
	else
		resolve_dependencies(task);
}

void thread_pool::add( task* task )
{
	// Add task to requested worker.
	if (NOT_A_THREAD_ID != task->thread_affinity)
	{
		workers[task->thread_affinity].add_task(task);
		return;
	}

	// Add task to least burdened worker.
	min_element(workers.begin(), workers.end(), [] ( worker& lhs, worker& rhs )
	{ return lhs.tasks.weight < rhs.tasks.weight; })->add_task(task);
}

void thread_pool::processed_task( task* task )
{
	resolve_dependencies(task);

	if (0 < --scheduled_task_count) return;

	lock_guard<mutex> lock(waiting_for_workers);
	all_tasks_are_processed.notify_all();
}

void thread_pool::resolve_dependencies( task* task )
{
  while (task)
  {
    if (!task->is_root())
    {
      int sub_tasks_left = --task->predecessor->sub_tasks_left;

      if (0 == sub_tasks_left)
      {
        if (task->predecessor->successor)
          digest_task(task->predecessor->successor.get());
        else
        {
          task = task->predecessor;
          continue;
        }
      }
      else if (-1 == sub_tasks_left)
      {
        task = task->predecessor;
        continue;
      }
    }
    else if (task->is_owned_by_thread_pool())
      delete task;
    
    task = nullptr;
  }
}

void thread_pool::stop_workers()
{
	for (auto worker = workers.begin(); workers.end() != worker; ++worker)
		worker->stop();
}

void thread_pool::wait_for_workers_to_stop()
{
	worker_threads.join_all();
}



////////////////////////////////////////////////////////////////////////////////
/// Worker
////////////////////////////////////////////////////////////////////////////////
void thread_pool::worker::wake()
{
	if (!about_to_wait) return;

	lock_guard<mutex> lock(waiting);
	work_to_do.notify_one();
}

void thread_pool::worker::operator()()
{
	task* task = nullptr;

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
		pool->processed_task(task);
	}
}

} // namespace thread_pool
} // namespace black_label
