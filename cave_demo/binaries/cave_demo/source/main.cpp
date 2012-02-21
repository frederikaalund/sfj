#include "cave_demo/application.hpp"

#include "black_label/thread_pool/tasks.hpp"



using namespace black_label::world;
using namespace black_label::thread_pool;
using namespace cave_demo;



int size = 1000*1000;
float
	* number1s,
	* number2s,
	* number3s;

// Simulates work
void task1( void* data )
{
	for (int i = 0; i < size; ++i)
		number3s[i] += number1s[i] * number2s[i];
}

// Creates and schedules other tasks
void task2( void* data )
{
	static int run = 0;
	if (run++ > 10) // Atomicity not guaranteed, but this is just a test task.
		return;

	application* demo = reinterpret_cast<application*>(data);
	
	int i = 0;
	tasks::size_type task = demo->thread_pool->create_and_schedule(task1, 0, NOT_A_THREAD_ID, 1);
	while (i++ < 5000)
		task = demo->thread_pool->create_and_schedule(task1, 0, NOT_A_THREAD_ID, 1);
}



int main( int argc, char const* argv[] )
{	
	application demo(argc, argv);
	if (!demo.is_fully_constructed()) return 1;



////////////////////////////////////////////////////////////////////////////////
/// World Test
////////////////////////////////////////////////////////////////////////////////
	world* world = demo.world;

	// Create some entities, then delete one, then create another one
	entities::size_type first_id = 
		world->static_entities.create(entities::matrix4f());
	world->static_entities.create(entities::matrix4f());
	world->static_entities.remove(first_id);
	world->static_entities.create(entities::matrix4f());



////////////////////////////////////////////////////////////////////////////////
/// Thread Pool Test
////////////////////////////////////////////////////////////////////////////////
	thread_pool* thread_pool = demo.thread_pool;

	// Allocate data
	number1s = new float[size],
	number2s = new float[size],
	number3s = new float[size];

	//thread_pool->current_thread_id();

	// Create an initial task and schedule it
	thread_pool->create_and_schedule(task2, &demo);
	// The thread pool will give work to the current thread
	thread_pool->employ_current_thread();
	// Blocks until all tasks have been processed
	thread_pool->join();

	// Cleanup
	delete[] number1s;
	delete[] number2s;
	delete[] number3s;



////////////////////////////////////////////////////////////////////////////////
/// Exit
////////////////////////////////////////////////////////////////////////////////
	return 0;
}
