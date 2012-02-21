#ifndef CAVE_DEMO_APPLICATION_HPP
#define CAVE_DEMO_APPLICATION_HPP

#include "black_label/log.hpp"
#include "black_label/shared_library.hpp"
#include "black_label/thread_pool/dynamic_interface.hpp"
#include "black_label/world/dynamic_interface.hpp"

#include <boost/program_options.hpp>



namespace cave_demo
{

class application
{
public:
	application( int argc, char const* argv[] );
	~application();

	bool is_fully_constructed();

	black_label::log::log* log;
	black_label::thread_pool::thread_pool* thread_pool;
	black_label::world::world* world;
protected:
private:
	void register_program_options( 
		int argc, 
        char const* argv[], 
		black_label::world::world::configuration& world_configuration );

	black_label::shared_library::shared_library
		* thread_pool_library,
		* world_library;

	bool fully_constructed;
	boost::program_options::variables_map program_options_map;
};

} // namespace cave_demo



#endif
