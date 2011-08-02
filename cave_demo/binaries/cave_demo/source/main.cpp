#include "cave_demo/application.hpp"

#include "black_label/thread_pool/task.hpp"



using namespace black_label::world;
using namespace black_label::thread_pool;
using namespace cave_demo;



int size = 1000*1000;
float
	* number1s,
	* number2s,
	* number3s;

void task1( void* data )
{
	for (int i = 0; i < size; ++i)
	{
		number3s[i] += number1s[i] * number2s[i];
	}
}

void task2( void* data )
{
	static int run = 0;
	if (run++ > 5)
		return;

	application* demo = reinterpret_cast<application*>(data);

	

	int i = 0;
	while (i++ < 500)
	{
		task temp_task(task1, 0, NOT_A_THREAD_ID, 0);
		thread_pool::add_task(demo->thread_pool, temp_task);
	}

	task task2_temp(task2, data, NOT_A_THREAD_ID, 0);
	thread_pool::add_task(demo->thread_pool, task2_temp);
}



int main( int argc, char const* argv[] )
{	
	application demo(argc, argv);
	if (!demo.is_fully_constructed()) goto error;



////////////////////////////////////////////////////////////////////////////////
/// Logic and Rendering
////////////////////////////////////////////////////////////////////////////////
	entities::size_type first_id = entities::add(
		demo.world->static_entities,
		entities::matrix4f());
	entities::add(
		demo.world->static_entities,
		entities::matrix4f());
	entities::remove(
		demo.world->static_entities,
		first_id);
	entities::add(
		demo.world->static_entities,
		entities::matrix4f());



////////////////////////////////////////////////////////////////////////////////
/// Thread Pool Test
////////////////////////////////////////////////////////////////////////////////

	// Allocate data
	number1s = new float[size],
	number2s = new float[size],
	number3s = new float[size];

	// Run
	task temp_task(task2, &demo, NOT_A_THREAD_ID, 0);
	thread_pool::add_task(demo.thread_pool, temp_task);
	thread_pool::join(demo.thread_pool);

	// Cleanup
	delete[] number1s;
	delete[] number2s;
	delete[] number3s;



////////////////////////////////////////////////////////////////////////////////
/// Exit
////////////////////////////////////////////////////////////////////////////////
	return 0;

	error:
	return 1;
}
