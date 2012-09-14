// Version 0.1
#ifndef BLACK_LABEL_THREAD_POOL_THREAD_POOL_HPP
#define BLACK_LABEL_THREAD_POOL_THREAD_POOL_HPP

#include <black_label/container/darray.hpp>
#include <black_label/shared_library/utility.hpp>
#include <black_label/thread_pool/task.hpp>
#include <black_label/thread_pool/types.hpp>
#include <black_label/utility/boost_atomic_extensions.hpp>

#include <boost/lockfree/fifo.hpp>
#define BOOST_THREAD_VERSION 2
#define BOOST_THREAD_DONT_PROVIDE_DEPRECATED_FEATURES_SINCE_V3_0_0
#include <boost/thread.hpp>
#include <boost/thread/condition_variable.hpp>



namespace black_label
{
namespace thread_pool
{

class BLACK_LABEL_SHARED_LIBRARY thread_pool
{
public:
	thread_pool();
	~thread_pool();

	void schedule( const task& task );
	void schedule( task* task );
	void join();
	void current_thread_id();



private:
MSVC_PUSH_WARNINGS(4251)
////////////////////////////////////////////////////////////////////////////////
/// Task Group
////////////////////////////////////////////////////////////////////////////////
	class task_group
	{
	public:
		typedef boost::lockfree::fifo<task*> task_queue_type;

		task_group() : weight(0) {}

		void add( task* task ) { weight += task->weight; tasks.enqueue(task); }
		bool next( task*& task ) { return tasks.dequeue(task); }
		void process( task* task ) { (*task)(); weight -= task->weight; }

		task_queue_type tasks;
		boost::atomic<task::weight_type> weight;
	};



////////////////////////////////////////////////////////////////////////////////
/// Worker
////////////////////////////////////////////////////////////////////////////////
	class worker 
	{
	public:
		worker()
			: pool()
			, about_to_wait(false)
			, work(true)
		{}

		void add_task( task* task ) { tasks.add(task); wake(); }
		void start() { work = true; (*this)(); }
		void stop() { work = false; wake(); }
		void wake();

		void operator()();

		thread_pool* pool;
		task_group tasks;
		boost::atomic<bool> 
			about_to_wait,
			work;
		boost::mutex waiting;
		boost::condition_variable work_to_do;
	};



	typedef container::darray<worker> worker_container;

	void digest_task( task* task );
	void add( task* task );
	void processed_task( task* task );
	void resolve_dependencies( task* task );
	void stop_workers();
	void wait_for_workers_to_stop();

	

	worker_container workers;
	boost::thread_group worker_threads;
	boost::atomic<int> scheduled_task_count;
	boost::mutex waiting_for_workers;
	boost::condition_variable all_tasks_are_processed;
MSVC_POP_WARNINGS()
};

} // namespace thread_pool
} // namespace black_label



#endif
