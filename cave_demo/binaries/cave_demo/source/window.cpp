#include <cave_demo/window.hpp>

#include <SFML/OpenGL.hpp>



namespace cave_demo {

window::window( int width, int height )
	: window_{sf::VideoMode{static_cast<unsigned int>(width), static_cast<unsigned int>(height)}, "OpenGL", 
		sf::Style::Titlebar | sf::Style::Close | sf::Style::Resize, 
		sf::ContextSettings{32, 0, 0, 4, 0}}
	, is_window_focused{true}
{ window_.setKeyRepeatEnabled(false); }

bool window::update()
{
	sf::Event event;
	while (window_.pollEvent(event))
	{
		switch (event.type)
		{
		case sf::Event::Closed:
			window_.close();
			break;
		case sf::Event::Resized:
			resize(event.size.width, event.size.height);
			break;

		case sf::Event::KeyPressed:
		case sf::Event::KeyReleased:
			switch (event.key.code)
			{
			case sf::Keyboard::Escape:
				window_.close();
				break;
			}
			break;

		case sf::Event::MouseButtonPressed:
		case sf::Event::MouseButtonReleased:
			break;

		case sf::Event::MouseMoved:
			on_mouse_moved(event.mouseMove.x, event.mouseMove.y);
			break;

		case sf::Event::GainedFocus:
			is_window_focused = true;
			break;
		case sf::Event::LostFocus:
			is_window_focused = false;
			break;

		default:
			break;
		}	
	}

	return is_open();
}

void window::resize( int width, int height ) {
	window_.setSize({static_cast<unsigned int>(width), static_cast<unsigned int>(height)});
	window_.setView(sf::View{sf::FloatRect{0.f, 0.f, static_cast<float>(width), static_cast<float>(height)}});
	on_resized(width, height);
}

} // namespace cave_demo
