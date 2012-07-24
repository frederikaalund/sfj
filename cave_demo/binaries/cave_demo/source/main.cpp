#include <cave_demo/application.hpp>

#include <memory>
#include <iostream>



using namespace black_label::world;
using namespace black_label::thread_pool;
using namespace cave_demo;



void wait()
{
	srand(535435);
	int max = rand()%2500;
	for (int i = 0; i < max; ++i)
		while(0 < rand()) int i = 2;
}



int main( int argc, char const* argv[] )
{	
	application demo(argc, argv); // TODO: Check for exceptions



////////////////////////////////////////////////////////////////////////////////
/// World Test
////////////////////////////////////////////////////////////////////////////////
	world& world = demo.world;

	// Create some entities, then delete one, then create another one
	world::generic_type::size_type first_id = 
		world.static_entities.create("cube");
	world.static_entities.create("cube");
	world.static_entities.remove(first_id);
	world.static_entities.create("cube");



////////////////////////////////////////////////////////////////////////////////
/// Thread Pool Test
////////////////////////////////////////////////////////////////////////////////
	thread_pool& thread_pool = demo.thread_pool;

	// Allocate data
	int size = 1000*1000;
	std::unique_ptr<float[]>
		number1s(new float[size]),
		number2s(new float[size]),
		number3s(new float[size]);

	using std::cout;
	using std::endl;
	
	task task_1 = ([&](){wait();cout<<"A1 ";} | [&](){wait();cout<<"A2 ";}) 
		>> ([&](){wait();cout<<"B1 ";} | [&](){wait();cout<<"B2 ";} | [&](){wait();cout<<"B3 ";})
		>> [&](){wait();cout<<"C1 ";};
	
	task task_2 = ([&](){wait();cout<<"E1 ";} | [&](){wait();cout<<"E2 ";})
		>> ([&](){wait();cout<<"F1 ";} | [&](){wait();cout<<"F2 ";});
		
	task task_3 = [&](){wait();cout<<"D1 ";} >> [&](){wait();cout<<"D2 ";};
	

	// Schedule a task
	thread_pool.schedule(task_1);
	// Blocks until all tasks have been processed
	thread_pool.join();


	
////////////////////////////////////////////////////////////////////////////////
/// Exit
////////////////////////////////////////////////////////////////////////////////
	return 0;
}
