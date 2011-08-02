#include "cave_demo/application.hpp"



namespace cave_demo
{

using namespace black_label::world;
using namespace black_label::shared_library;
using namespace black_label::thread_pool;
using namespace boost::program_options;



application::application( int argc, char const* argv[] )
	: log(new black_label::log::log("log.txt"))
	, thread_pool(0)
	, world(0)
	, world_library(0)
	, thread_pool_library(0)
	, fully_constructed(false)
{
	if (!log->is_open()) return;

	char* world_function_names[] = {
		"construct_world",
		"destroy_world",
		"copy_world",
		"add_entity",
		"remove_entity"};

	void** world_function_pointers[] = {
		reinterpret_cast<void**>(&world::construct),
		reinterpret_cast<void**>(&world::destroy),
		reinterpret_cast<void**>(&world::copy),
		reinterpret_cast<void**>(&entities::add),
		reinterpret_cast<void**>(&entities::remove)};

	world_library = new shared_library(
		"D:/sfj/black_label/stage/libraries/black_label_world-vc100-mt-gd-0_1.dll",
		log,
		reinterpret_cast<shared_library::write_to_log_type*>(
		&black_label::log::log::write_to_log));

	if (!world_library->is_open()) return;

	if (0 < 
		world_library->map_symbols(
			5,
			world_function_names,
			world_function_pointers))
		return;





	char* thread_pool_function_names[] = {
		"construct_thread_pool",
		"destroy_thread_pool",
		"add_task",
		"add_raw_task",
		"join"};

	void** thread_pool_function_pointers[] = {
		reinterpret_cast<void**>(&thread_pool::construct),
		reinterpret_cast<void**>(&thread_pool::destroy),
		reinterpret_cast<void**>(&thread_pool::add_task),
		reinterpret_cast<void**>(&thread_pool::add_raw_task),
		reinterpret_cast<void**>(&thread_pool::join)};

	thread_pool_library = new shared_library(
		"D:/sfj/black_label/stage/libraries/black_label_thread_pool-vc100-mt-gd-0_1.dll",
		log,
		reinterpret_cast<shared_library::write_to_log_type*>(
		&black_label::log::log::write_to_log));

	if (!thread_pool_library->is_open()) return;

	if (0 < 
		thread_pool_library->map_symbols(
			5,
			thread_pool_function_names,
			thread_pool_function_pointers))
		return;

	world::configuration world_configuration(5000, 5000);
	register_program_options(argc, argv, world_configuration);

	world = world::construct( world_configuration );

	thread_pool = thread_pool::construct();

	fully_constructed = true;
}

application::~application()
{
	if (thread_pool) thread_pool::destroy(thread_pool);
	if (world) world::destroy(world);
	if (thread_pool_library) delete thread_pool_library;
	if (world_library) delete world_library;
	delete log;
}



bool application::is_fully_constructed()
{
	return fully_constructed;
}



void application::register_program_options( 
	int argc, 
	const char* argv[],
	world::configuration& world_configuration )
{
	options_description description("Options");
	description.add_options()
		("world.entities.dynamic_capacity", 
			value<entities::size_type>(
				&world_configuration.dynamic_entities_capacity)
					->default_value(
						world_configuration.dynamic_entities_capacity))
		("world.entities.static_capacity", 
			value<entities::size_type>(
				&world_configuration.static_entities_capacity)
					->default_value(
					world_configuration.static_entities_capacity));
	program_options_map;

	try
	{
		store(
			parse_command_line(argc, argv, description), 
			program_options_map);
		notify(program_options_map);
	}
	// Apparently, this specific error does not pass the same value to the
	// generic .what().
	catch (invalid_option_value e)
	{
		log->write(
			1, 
			"Skipping command line options due to error: \"%s\".", e.what());
	}
	catch (error e)
	{
		log->write(
			1, 
			"Skipping command line options due to error: \"%s\".", e.what());
	}

	try
	{
		store(
			parse_config_file<char>("configuration.ini", description), 
			program_options_map);
		notify(program_options_map);
	}
	catch (error e)
	{
		log->write(
			1, 
			"Skipping \"configuration.ini\" due to error: \"%s\".", e.what());
	}
}

} // namespace cave_demo
