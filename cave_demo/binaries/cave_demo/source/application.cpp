#include <cave_demo/application.hpp>



namespace cave_demo
{

using namespace black_label::world;
using namespace black_label::thread_pool;
using namespace boost::program_options;



application::application( int argc, char const* argv[] )
	: log("log.txt")
{
	world::configuration world_configuration(5000, 5000);
	register_program_options(argc, argv, world_configuration);

	world = black_label::world::world(world_configuration);
}

application::~application()
{
}



void application::register_program_options( 
	int argc, 
    char const* argv[],
	world::configuration& world_configuration )
{
	options_description description("Options");
	description.add_options()
		("world.entities.dynamic_capacity", 
			value<world::generic_type::size_type>(
				&world_configuration.dynamic_entities_capacity)
					->default_value(
						world_configuration.dynamic_entities_capacity))
		("world.entities.static_capacity", 
			value<world::generic_type::size_type>(
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
		log.write(
			1, 
			"Skipping command line options due to error: \"%s\".", e.what());
	}
	catch (error e)
	{
		log.write(
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
		log.write(
			1, 
			"Skipping \"configuration.ini\" due to error: \"%s\".", e.what());
	}
}

} // namespace cave_demo
