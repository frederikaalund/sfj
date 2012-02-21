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
	, thread_pool_library(0)
	, world_library(0)
	, fully_constructed(false)
{
	if (!log->is_open()) return;



#ifdef WINDOWS
    #define WORLD_LIBRARY_PATH "../../../black_label/stage/libraries/black_label_world-vc100-mt-gd-0_1.dll"
#elif UNIX
    #define WORLD_LIBRARY_PATH "/Users/frederikaalund/Documents/sfj/black_label/stage/libraries/libblack_label_world-xgcc421-gd-0_1.so"
#else
    #error "Unsupported platform!"
#endif
    
#define WORLD_LIBRARY_SYMBOL_COUNT 6

	char const* world_function_names[WORLD_LIBRARY_SYMBOL_COUNT] = {
		"world_construct",
		"world_destroy",
		"world_dynamic_entities",
		"world_static_entities",
		"entities_create",
		"entities_remove"};

	void** world_function_pointers[WORLD_LIBRARY_SYMBOL_COUNT] = {
		reinterpret_cast<void**>(&world::method_pointers.construct),
		reinterpret_cast<void**>(&world::method_pointers.destroy),
		reinterpret_cast<void**>(&world::method_pointers.dynamic_entities),
		reinterpret_cast<void**>(&world::method_pointers.static_entities),
		reinterpret_cast<void**>(&entities::method_pointers.create),
		reinterpret_cast<void**>(&entities::method_pointers.remove)};

	world_library = new shared_library(
        WORLD_LIBRARY_PATH,
		log,
		reinterpret_cast<shared_library::write_to_log_type*>(
		&black_label::log::log::write_to_log));

	if (!world_library->is_open()) return;

	if (0 < 
		world_library->map_symbols(
			WORLD_LIBRARY_SYMBOL_COUNT,
			world_function_names,
			world_function_pointers))
		return;



#ifdef WINDOWS
#define THREAD_POOL_LIBRARY_PATH "../../../black_label/stage/libraries/black_label_thread_pool-vc100-mt-gd-0_1.dll"
#elif UNIX
#define THREAD_POOL_LIBRARY_PATH "/Users/frederikaalund/Documents/sfj/black_label/stage/libraries/libblack_label_thread_pool-xgcc421-gd-0_1.so"
#else
#error "Unsupported platform!"
#endif
    
#define THREAD_POOL_LIBRARY_SYMBOL_COUNT 11

	char const* thread_pool_function_names[THREAD_POOL_LIBRARY_SYMBOL_COUNT] = {
		"thread_pool_construct",
		"thread_pool_destroy",
		"thread_pool_schedule",
		"thread_pool_create_and_schedule",
		"thread_pool_employ_current_thread",
		"thread_pool_join",
		"thread_pool_tasks",
		"tasks_construct",
		"tasks_destroy",
		"tasks_create",
		"tasks_remove"};

	void** thread_pool_function_pointers[THREAD_POOL_LIBRARY_SYMBOL_COUNT] = {
		reinterpret_cast<void**>(&thread_pool::method_pointers.construct),
		reinterpret_cast<void**>(&thread_pool::method_pointers.destroy),
		reinterpret_cast<void**>(&thread_pool::method_pointers.schedule),
		reinterpret_cast<void**>(&thread_pool::method_pointers.create_and_schedule),
		reinterpret_cast<void**>(&thread_pool::method_pointers.employ_current_thread),
		reinterpret_cast<void**>(&thread_pool::method_pointers.join),
		reinterpret_cast<void**>(&thread_pool::method_pointers.tasks),
		reinterpret_cast<void**>(&tasks::method_pointers.construct),
		reinterpret_cast<void**>(&tasks::method_pointers.destroy),
		reinterpret_cast<void**>(&tasks::method_pointers.create),
		reinterpret_cast<void**>(&tasks::method_pointers.remove)};

	thread_pool_library = new shared_library(
		THREAD_POOL_LIBRARY_PATH,
		log,
		reinterpret_cast<shared_library::write_to_log_type*>(
		&black_label::log::log::write_to_log));

	if (!thread_pool_library->is_open()) return;

	if (0 < 
		thread_pool_library->map_symbols(
			THREAD_POOL_LIBRARY_SYMBOL_COUNT,
			thread_pool_function_names,
			thread_pool_function_pointers))
		return;

	world::configuration world_configuration(5000, 5000);
	register_program_options(argc, argv, world_configuration);

	world = new black_label::world::world(world_configuration);

	thread_pool = new black_label::thread_pool::thread_pool();

	fully_constructed = true;
}

application::~application()
{
	if (thread_pool) delete thread_pool;
	if (world) delete world;
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
    char const* argv[],
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
