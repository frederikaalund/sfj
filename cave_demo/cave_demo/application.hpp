#ifndef CAVE_DEMO_APPLICATION_HPP
#define CAVE_DEMO_APPLICATION_HPP

#include <black_label/dynamics.hpp>
#include <black_label/file_system_watcher.hpp>
#include <black_label/utility/log_severity_level.hpp>
#include <black_label/renderer.hpp>
#include <black_label/thread_pool.hpp>
#include <black_label/world.hpp>

#include <boost/log/sources/severity_logger.hpp>

#include <SFML/Window.hpp>



namespace cave_demo {

class application
{
private:
	typedef boost::log::sources::severity_logger<black_label::utility::severity_level> log_type;
	log_type log;



public:
////////////////////////////////////////////////////////////////////////////////
/// Configuration
////////////////////////////////////////////////////////////////////////////////
	struct configuration 
	{
		configuration( log_type& log, int argc, char const* argv[] );

		black_label::world::world::configuration world_configuration;
	} configuration;



	application( int argc, char const* argv[] );
	~application();

	void update_window();
	void update_file_system_watcher();
	void update() { update_window(); update_file_system_watcher(); }

	bool window_is_open() { return window.isOpen(); }

	sf::Window window;
  
	black_label::thread_pool::thread_pool thread_pool;
	black_label::world::world world;
	black_label::renderer::renderer renderer;
	black_label::dynamics::dynamics dynamics;
	


protected:
private:
	void process_keyboard_event( const sf::Event& event );
	void process_mouse_movement_event( const sf::Event& event );
	void process_mouse_button_event( const sf::Event& event );
	
	black_label::file_system_watcher::file_system_watcher fsw;
	
	float increment;
	glm::vec3 strafe;
	int last_x, last_y;
};

} // namespace cave_demo



#endif
