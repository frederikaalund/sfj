#include <cave_demo/application.hpp>

#include <iostream>
#include <ostream>
#include <sstream>
#include <tuple>

#include <boost/log/attributes.hpp>
#include <boost/log/core.hpp>
#include <boost/log/common.hpp>
#include <boost/log/filters.hpp>
#include <boost/log/formatters.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/program_options.hpp>

#include <SFML/OpenGL.hpp>



namespace black_label {
namespace utility {

template<typename CharT, typename TraitsT>
inline std::basic_ostream< CharT, TraitsT >& operator<<(
	std::basic_ostream<CharT, TraitsT>& stream, 
	severity_level level )
{
	static const char* strings[] =
	{
		"info",
		"warning",
		"error"
	};

	if (static_cast<std::size_t>(level) < sizeof(strings) / sizeof(*strings))
		stream << strings[level];
	else
		stream << static_cast<int>(level);

	return stream;
}

}
}



namespace cave_demo {

using namespace black_label::file_system_watcher;
using namespace black_label::renderer;
using namespace black_label::thread_pool;
using namespace black_label::world;
using namespace black_label::utility;

using namespace boost::log;
using namespace boost::program_options;
using boost::make_shared;



////////////////////////////////////////////////////////////////////////////////
/// Configuration
////////////////////////////////////////////////////////////////////////////////
application::configuration::configuration( log_type& log, int argc, char const* argv[] )
{
////////////////////////////////////////////////////////////////////////////////
/// Logging
////////////////////////////////////////////////////////////////////////////////
	auto core = boost::log::core::get();


	// File
	auto file_backend = make_shared<sinks::text_file_backend>(
		keywords::file_name = "./logs/log_%N.txt",
		keywords::rotation_size = 5 * 1024 * 1024);
	file_backend->set_file_collector(sinks::file::make_collector(
		keywords::target = "./logs/",
		keywords::max_size = 10 * 1024 * 1024, // 10 MiB
		keywords::min_free_space = 50 * 1024 * 1024)); // 50 MiB
	file_backend->scan_for_files();

	auto file_sink = make_shared<sinks::synchronous_sink<sinks::text_file_backend> >(
		file_backend);

	file_sink->set_formatter(
		formatters::stream
		<< "[" << formatters::attr<severity_level>("Severity")
		<< "] " << formatters::date_time<boost::posix_time::ptime>("TimeStamp", "%d.%m.%Y %H:%M:%S.%f")
		<< " (" << formatters::time_duration<boost::posix_time::time_duration>("Uptime") << "):"
		<< formatters::if_(filters::has_attr<bool>("MultiLine"))
		[ formatters::stream << "\n" ]
	.else_
		[ formatters::stream << " " ]
	<< formatters::message());


	// Console
	auto clog_backend = make_shared<sinks::text_ostream_backend>();
	clog_backend->add_stream(boost::shared_ptr<std::ostream>(&std::clog, empty_deleter()));
	clog_backend->auto_flush(true);

	auto clog_sink = make_shared<sinks::synchronous_sink<sinks::text_ostream_backend> >(
		clog_backend);

	clog_sink->set_formatter(
		formatters::stream
		<< "[" << formatters::attr<severity_level>("Severity")
		<< "] " << formatters::time_duration<boost::posix_time::time_duration>("Uptime", "%M:%S.%f") << ":"
		<< formatters::if_(filters::has_attr<bool>("MultiLine"))
		[ formatters::stream << "\n" ]
	.else_
		[ formatters::stream << " " ]
	<< formatters::message());

	core->add_global_attribute("TimeStamp", attributes::local_clock());
	core->add_global_attribute("Uptime", attributes::timer());
	core->add_sink(file_sink);
	core->add_sink(clog_sink);



////////////////////////////////////////////////////////////////////////////////
/// Configuration
////////////////////////////////////////////////////////////////////////////////
	boost::program_options::variables_map program_options_map;
	world_configuration = world::configuration(5000, 5000);



	options_description description("Options");
	description.add_options()
		("world.entities.dynamic_capacity", 
		value<world::entities_type::size_type>(
		&world_configuration.dynamic_entities_capacity)
		->default_value(
		world_configuration.dynamic_entities_capacity))
		("world.entities.static_capacity", 
		value<world::entities_type::size_type>(
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
	// generic error.what().
	catch (invalid_option_value e)
	{
		BOOST_LOG_SEV(log, warning) 
			<< "Skipping command line options due to error: \"" << e.what()
			<< "\"";
	}
	catch (boost::program_options::error e)
	{
		BOOST_LOG_SEV(log, warning) 
			<< "Skipping command line options due to error: \"" << e.what()
			<< "\"";
	}

	try
	{
		store(
			parse_config_file<char>("configuration.ini", description), 
			program_options_map);
		notify(program_options_map);
	}
	catch (boost::program_options::error e)
	{
		BOOST_LOG_SEV(log, warning) 
			<< "Skipping \"configuration.ini\" due to error: \"" << e.what()
			<< "\"";
	}
}



////////////////////////////////////////////////////////////////////////////////
/// Application
////////////////////////////////////////////////////////////////////////////////
application::application( int argc, char const* argv[] )
	: configuration(log, argc, argv)
	, window(sf::VideoMode(800, 800), "OpenGL", 
		sf::Style::Titlebar | sf::Style::Close | sf::Style::Resize, 
		sf::ContextSettings(32, 0, 0, 3, 2))
	, world(configuration.world_configuration)
	, renderer(world, camera(glm::vec3(1200.0f, 200.0f, 200.0f), 
		glm::vec3(1300.0f, 200.0f, -300.0f), glm::vec3(0.0f, 1.0f, 0.0f)))
	, dynamics(world)
	, fsw("D:/sfj/cave_demo/stage/binaries", FILTER_WRITE)
	, increment(1.0f)
	, strafe(0.0f)
{
	window.setKeyRepeatEnabled(false);
}

application::~application()
{
}



////////////////////////////////////////////////////////////////////////////////
/// Process Events
////////////////////////////////////////////////////////////////////////////////
void application::process_keyboard_event( const sf::Event& event )
{
	float direction = (sf::Event::KeyPressed == event.type) ? 1.0f : -1.0f;
	
	switch (event.key.code)
	{
	case sf::Keyboard::Escape:
		{ exit(0); }
		break;
	case sf::Keyboard::W:
		{ strafe[2] += increment * direction; }
		break;
	case sf::Keyboard::S:
		{ strafe[2] -= increment * direction; }
		break;
	case sf::Keyboard::A:
		{ strafe[0] += increment * direction; }
		break;
	case sf::Keyboard::D:
		{ strafe[0] -= increment * direction; }
		break;
	case sf::Keyboard::Space:
		{ strafe[1] += increment * direction; }
		break;
	case sf::Keyboard::C:
		{ strafe[1] -= increment * direction; }
		break;
  default:
    break;
	}
}

void application::process_mouse_movement_event( const sf::Event& event )
{		
	if (!sf::Mouse::isButtonPressed(sf::Mouse::Left)) 
	{
		last_x = event.mouseMove.x;
		last_y = event.mouseMove.y;
		return;
	}

	float 
		dx = static_cast<float>(last_x-event.mouseMove.x),
		dy = static_cast<float>(last_y-event.mouseMove.y);

	renderer.camera.pan(dx, dy);

	last_x = event.mouseMove.x;
	last_y = event.mouseMove.y;
}

void application::process_mouse_button_event( const sf::Event& event )
{
}



void application::update_window()
{
	sf::Event event;
	while (window.pollEvent(event))
	{
		switch (event.type)
		{
		case sf::Event::Closed:
			{ window.close(); return; }
			break;
		case sf::Event::Resized:
			{ renderer.on_window_resized(event.size.width, event.size.height); }
			break;

		case sf::Event::KeyPressed:
		case sf::Event::KeyReleased:
			{ process_keyboard_event(event); }
			break;

		case sf::Event::MouseButtonPressed:
		case sf::Event::MouseButtonReleased:
			{ process_mouse_button_event(event); }
			break;

		case sf::Event::MouseMoved:
			{ process_mouse_movement_event(event); }
			break;

		default:
			break;
		}	
	}

	dynamics.update();
	renderer.camera.strafe(strafe);
	renderer.render_frame();
	//renderer.render_frame_();
	window.display();

	static int frames = 0;
	static sf::Clock clock;
	if (frames++ < 1000)
	{
		sf::Time elapsed_time = clock.restart();
		if (sf::Time::Zero < elapsed_time)
		{
			std::stringstream ss;
			ss << "FPS: " << 1000000/elapsed_time.asMicroseconds();
			window.setTitle(ss.str());
		}

		frames = 0;
	}
}

void application::update_file_system_watcher()
{
	try
	{
		auto& models = this->world.static_entities.models;
		auto& renderer = this->renderer;
		auto& log = this->log;

		typedef std::tuple<path_type, world::model_container::const_iterator, sf::Clock> path_pair;
		static std::vector<path_pair> watched_paths;

		fsw.update();
		std::for_each(fsw.modified_paths.cbegin(), fsw.modified_paths.cend(),
			[&] ( const path_type& path ) {

			auto found = std::find(models.cbegin(),	models.cend(), path);

			// Watch any model files that have been modified.
			if (models.cend() != found)
			{
				auto current_pair = std::make_tuple(*found, found, sf::Clock());
				auto found_pair = std::find_if(watched_paths.begin(), watched_paths.end(),
					[&] ( path_pair& pp ) -> bool { return std::get<0>(pp) == std::get<0>(current_pair); });

				if (watched_paths.end() != found_pair)
					std::get<2>(*found_pair).restart();
				else
					watched_paths.push_back(current_pair);
			}
		});

		// If a watched model file has not been modified for 1.0 seconds then reload it.
		auto new_end = std::partition(watched_paths.begin(), watched_paths.end(),
			[&] ( path_pair& pp ) -> bool { return 1.0f >= std::get<2>(pp).getElapsedTime().asSeconds(); });

		std::for_each(new_end, watched_paths.end(), [&] ( path_pair& pp ) {
			renderer.report_dirty_model(std::get<1>(pp));
			BOOST_LOG_SEV(log, info) << "Dirty entity <id: " << *std::get<1>(pp) << ", model: \"" << std::get<0>(pp) << "\">";
		});
		watched_paths.resize(new_end - watched_paths.begin());

		fsw.modified_paths.clear();
	}
	catch (const std::exception& error)
	{
		BOOST_LOG_SEV(log, black_label::utility::error) << error.what();
	}
}

} // namespace cave_demo
