#ifndef CAVE_DEMO_APPLICATION_HPP
#define CAVE_DEMO_APPLICATION_HPP

#include <black_label/file_system_watcher.hpp>
#include <black_label/rendering.hpp>
#include <black_label/world/entities.hpp>

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

//#include <ittnotify.h>
//#define tbb_use_threading_tools 1
//#define tbb_use_performance_warnings 1
//#include <tbb/task_scheduler_init.h>
//#include <tbb/tbb_stddef.h>



namespace cave_demo {

class application
{
public:
////////////////////////////////////////////////////////////////////////////////
/// Configuration
////////////////////////////////////////////////////////////////////////////////
	class configuration
	{
	public:
		configuration( int argc, char const* argv[] );
		black_label::path shader_directory, asset_directory;
	} configuration;



	using rendering_assets_type = black_label::rendering::assets<
		int, 
		black_label::world::entities::model_type*, 
		black_label::world::entities::transformation_type*>;

	application( int argc, char const* argv[] );
	application( const application& other ) = delete;

	void update_window();
	void update_file_system_watcher();
	void update()
	{ 
		update_window(); 
		if (!window_is_open()) return;
		update_file_system_watcher(); 
	}

	bool window_is_open() { return window.isOpen(); }

	sf::RenderWindow window;
	sf::Font font;
	sf::Text text;
  
	black_label::world::group all_statics;
	black_label::rendering::view view;
	black_label::rendering::setup setup;
	black_label::rendering::gpu::framebuffer framebuffer;
	rendering_assets_type rendering_assets;
	black_label::rendering::pipeline rendering_pipeline;
	


protected:
private:
	void process_keyboard_event( const sf::Event& event );
	void process_mouse_movement_event( const sf::Event& event );
	void process_mouse_button_event( const sf::Event& event );
	void update_movement();
	
	black_label::file_system_watcher::file_system_watcher fsw;
	
	float increment;
	glm::vec3 strafe;
	int last_x, last_y;
	bool is_window_focused;
};

} // namespace cave_demo



#endif
