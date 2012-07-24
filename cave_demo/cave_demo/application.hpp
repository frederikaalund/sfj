#ifndef CAVE_DEMO_APPLICATION_HPP
#define CAVE_DEMO_APPLICATION_HPP

#include <black_label/log.hpp>
#include <black_label/thread_pool.hpp>
#include <black_label/world.hpp>

#include <boost/program_options.hpp>



namespace cave_demo
{

class application
{
public:
	application( int argc, char const* argv[] );
	~application();

	black_label::log::log log;
	black_label::thread_pool::thread_pool thread_pool;
	black_label::world::world world;
protected:
private:
	void register_program_options( 
		int argc, 
        char const* argv[], 
		black_label::world::world::configuration& world_configuration );

	boost::program_options::variables_map program_options_map;
};

} // namespace cave_demo



#endif
