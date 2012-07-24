// Version 0.1
#ifndef BLACK_LABEL_THREAD_POOL_THREAD_POOL_HPP
#define BLACK_LABEL_THREAD_POOL_THREAD_POOL_HPP

#include <black_label/container/darray.hpp>
#include <black_label/shared_library/utility.hpp>
#include <black_label/thread_pool/task.hpp>
#include <black_label/thread_pool/types.hpp>
#include <black_label/utility/boost_atomic_extensions.hpp>

#include <boost/lockfree/fifo.hpp>
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

	void thread_pool::schedule( const task& task );
	//void thread_pool::schedule( task& task ); // TODO: Externally managed task.
	void join();
	void current_thread_id();



private:
////////////////////////////////////////////////////////////////////////////////
/// Task Group
////////////////////////////////////////////////////////////////////////////////
	struct task_group
	{
		typedef boost::lockfree::fifo<task*> task_queue_type;

		task_group() : weight(0) {}

		void add( task* task );
		bool next( task*& task );
		void process( task* task );

		task_queue_type tasks;
		boost::atomic<task::weight_type> weight;
	};

	friend struct task_group;



////////////////////////////////////////////////////////////////////////////////
/// Worker
////////////////////////////////////////////////////////////////////////////////
	struct worker 
	{
		worker() 	
			: pool(pool)
			, about_to_wait(false)
			, work(true)
		{}

		void add_task( task* task );
		void start();
		void stop();
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

	void stop_workers();
	void wait_for_workers_to_stop();
	void resolve_dependencies( task* task );
	void processed_task( task* task );
	void add( task* task );

	worker_container workers;
	boost::thread_group worker_threads;
	boost::atomic<int> scheduled_task_count;
	boost::mutex waiting_for_workers;
	boost::condition_variable all_tasks_are_processed;

private:
	void thread_pool::schedule( task* task );
};

} // namespace thread_pool
} // namespace black_label



#endif
