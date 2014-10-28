#ifndef CAVE_DEMO_WINDOW_HPP
#define CAVE_DEMO_WINDOW_HPP

#include <functional>

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>



namespace cave_demo {

class window
{
public:
	window( int width, int height );
	window( const window& other ) = delete;;

	bool update();
	bool is_open() const { return window_.isOpen(); }
	bool is_focused() const { return is_window_focused; }

	void resize( int width, int height );

	sf::RenderWindow window_;
	std::function<void ( int width, int height )> on_resized;
	std::function<void ( int x, int y )> on_mouse_moved;

private:
	bool is_window_focused;
};

} // namespace cave_demo



#endif
