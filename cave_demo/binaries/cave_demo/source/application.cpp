#include <cave_demo/application.hpp>

#include <iostream>
#include <ostream>
#include <sstream>
#include <tuple>

#include <boost/log/attributes.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/program_options.hpp>
#include <boost/range/numeric.hpp>
#include <boost/utility/empty_deleter.hpp>

#include <SFML/OpenGL.hpp>



namespace cave_demo {

using namespace black_label;
using namespace black_label::file_system_watcher;
using namespace black_label::rendering;
using namespace black_label::utility;
using namespace black_label::world;

using boost::make_shared;



////////////////////////////////////////////////////////////////////////////////
/// Configuration
////////////////////////////////////////////////////////////////////////////////
application::configuration::configuration( int argc, char const* argv[] )
{
////////////////////////////////////////////////////////////////////////////////
/// Logging
////////////////////////////////////////////////////////////////////////////////
	using namespace boost::log;

	auto core = boost::log::core::get();


	// File
	auto file_backend = make_shared<sinks::text_file_backend>(
		keywords::file_name = "./logs/log_%N.txt",
		keywords::rotation_size = 5 * 1024 * 1024);
	//file_backend->set_file_collector(sinks::file::make_collector(
	//	keywords::target = "./logs/",
	//	keywords::max_size = 10 * 1024 * 1024, // 10 MiB
	//	keywords::min_free_space = 50 * 1024 * 1024)); // 50 MiB	
	//file_backend->scan_for_files();

	auto file_sink = make_shared<sinks::synchronous_sink<sinks::text_file_backend> >(
		file_backend);

	file_sink->set_formatter(
		expressions::stream
		<< "[" << expressions::attr<trivial::severity_level>("Severity")
		<< "] " << expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%d.%m.%Y %H:%M:%S.%f")
		<< " (" << expressions::format_date_time<boost::posix_time::time_duration>("Uptime", "%H:%M:%S.%f") << "):"
		<< expressions::if_(expressions::has_attr<bool>("MultiLine"))
		[expressions::stream << "\n"]
	.else_
		[expressions::stream << " "]
	<< expressions::message);


	// Console
	auto clog_backend = make_shared<sinks::text_ostream_backend>();
	clog_backend->add_stream(boost::shared_ptr<std::ostream>(&std::clog, boost::empty_deleter()));
	clog_backend->auto_flush(true);

	auto clog_sink = make_shared<sinks::synchronous_sink<sinks::text_ostream_backend> >(
		clog_backend);

	clog_sink->set_formatter(
		expressions::stream
		<< "[" << expressions::attr<trivial::severity_level>("Severity")
		<< "] " << expressions::format_date_time<boost::posix_time::time_duration>("Uptime", "%M:%S.%f") << ":"
		<< expressions::if_(expressions::has_attr<bool>("MultiLine"))
		[expressions::stream << "\n"]
	.else_
		[expressions::stream << " "]
	<< expressions::message);

	core->add_global_attribute("TimeStamp", attributes::local_clock());
	core->add_global_attribute("Uptime", attributes::timer());
	core->add_sink(file_sink);
	core->add_sink(clog_sink);



////////////////////////////////////////////////////////////////////////////////
/// Configuration
////////////////////////////////////////////////////////////////////////////////
	namespace po = boost::program_options;
	

	po::options_description general_options("General");
	general_options.add_options()
		(
			"help,?", "Print the help message."
		);

	po::options_description rendering_options("Renderer");
	
	rendering_options.add_options()
		(
			"rendering.shader_directory", 
			po::value<path>(&this->shader_directory)
		)(
			"rendering.asset_directory", 
			po::value<path>(&this->asset_directory)
		);

	po::options_description subsystem_options, all_options;
	subsystem_options.add(rendering_options);
	all_options.add(general_options).add(subsystem_options);

	po::variables_map variables_map;
	static const path configuration_file_path = "configuration.ini";

	try
	{
		store(po::parse_command_line(argc, argv, all_options), variables_map);
		store(
			po::parse_config_file<char>(
				configuration_file_path.string().c_str(), 
				subsystem_options), 
			variables_map);

		if (variables_map.count("help"))
			BOOST_LOG_TRIVIAL(info) << all_options;

		notify(variables_map);
	}
	catch (const po::required_option& e)
	{
		BOOST_LOG_TRIVIAL(error) << e.what();
		throw std::runtime_error("Missing required options!");
	}
	catch (const po::reading_file&)
	{
		BOOST_LOG_TRIVIAL(warning) 
			<< "Could not open the configuration file: " 
			<< configuration_file_path;
	}
	catch (const po::unknown_option& e)
	{
		BOOST_LOG_TRIVIAL(warning) << e.what();
	}
	catch (const po::error& e)
	{
		BOOST_LOG_TRIVIAL(warning) << e.what();
	}
}



////////////////////////////////////////////////////////////////////////////////
/// Application
////////////////////////////////////////////////////////////////////////////////
// Lion cam: view(glm::vec3(1200.0f, 200.0f, 200.0f), glm::vec3(1300.0f, 200.0f, -300.0f), glm::vec3(0.0f, 1.0f, 0.0f))
// Deep cam: view(glm::vec3(1200.0f, 200.0f, 200.0f), glm::vec3(300.0f, 200.0f, -300.0f), glm::vec3(0.0f, 1.0f, 0.0f));

application::application( int argc, char const* argv[] )
	: configuration{argc, argv}
	, window{sf::VideoMode{800, 800}, "OpenGL", 
		sf::Style::Titlebar | sf::Style::Close | sf::Style::Resize, 
		sf::ContextSettings{32, 0, 0, 3, 2}}
	, view{glm::vec3{1200.0f, 200.0f, 200.0f}, glm::vec3{300.0f, 200.0f, -300.0f}, glm::vec3{0.0f, 1.0f, 0.0f}, static_cast<int>(window.getSize().x), static_cast<int>(window.getSize().y)}
	, framebuffer{generate}
	, rendering_assets{configuration.asset_directory}
	, rendering_pipeline{configuration.shader_directory / "rendering_pipeline.json", configuration.shader_directory, view}
	, increment{1.0f}
	, strafe{0.0f}
	, is_window_focused{true}
{ 
	if (!font.loadFromFile((configuration.asset_directory / "fonts/Vegur-Regular.ttf").string())) {
		BOOST_LOG_TRIVIAL(error) << "Error loading font.";
		return;
	}
	text.setFont(font);

	fsw = {
		{configuration.asset_directory, filter::write | filter::access | filter::file_name},
		{configuration.shader_directory, filter::write | filter::access | filter::file_name}
	};
	window.setKeyRepeatEnabled(false);
}



////////////////////////////////////////////////////////////////////////////////
/// Process Events
////////////////////////////////////////////////////////////////////////////////
void application::process_keyboard_event( const sf::Event& event )
{
	switch (event.key.code)
	{
	case sf::Keyboard::Escape:
		{ window.close(); }
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

	view.pan(dx, dy);

	last_x = event.mouseMove.x;
	last_y = event.mouseMove.y;
}

void application::process_mouse_button_event( const sf::Event& event )
{
}

void application::update_movement()
{
	strafe = glm::vec3(0.0f);

	if (!is_window_focused)
		return;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
		strafe.z += increment;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
		strafe.z -= increment;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
		strafe.x += increment;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
		strafe.x -= increment;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
		strafe.y += increment;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::C))
		strafe.y -= increment;
}



void application::update_window()
{
	using namespace std;
	using namespace std::chrono;

	sf::Event event;
	while (window.pollEvent(event))
	{
		switch (event.type)
		{
		case sf::Event::Closed:
			{ window.close(); return; }
			break;
		case sf::Event::Resized:
			{ 
				window.setView(sf::View{sf::FloatRect{0.f, 0.f, static_cast<float>(event.size.width), static_cast<float>(event.size.height)}});
				view.on_window_resized(event.size.width, event.size.height);
				rendering_pipeline.on_window_resized(event.size.width, event.size.height);
			}
			break;

		case sf::Event::KeyPressed:
		case sf::Event::KeyReleased:
			{ 
				process_keyboard_event(event); 
				if (!window_is_open()) return;
			}
			break;

		case sf::Event::MouseButtonPressed:
		case sf::Event::MouseButtonReleased:
			{ process_mouse_button_event(event); }
			break;

		case sf::Event::MouseMoved:
			{ process_mouse_movement_event(event); }
			break;

		case sf::Event::GainedFocus:
			{ is_window_focused = true; }
			break;
		case sf::Event::LostFocus:
			{ is_window_focused = false; }
			break;

		default:
			break;
		}	
	}

	update_movement();
	view.strafe(strafe);
	rendering_assets.update();
	rendering_pipeline.render(framebuffer, rendering_assets);
	
	gpu::vertex_array::unbind();
	gpu::buffer::unbind();
	gpu::framebuffer::unbind();
	window.resetGLStates();

	static bool has_written_message{false};
	if (rendering_pipeline.is_complete()) {
		has_written_message = false;
		static unordered_map<string, double> averages;
		stringstream ss;
		ss.precision(3);
		auto output_pass = [&] ( const std::string& name, const chrono::high_resolution_clock::duration& render_duration )
		{ 
			if (10s < render_duration || 10us > render_duration) return;
			auto render_time = duration_cast<duration<double, milli>>(render_duration).count();
			static const double alpha{0.05};
			averages[name] = alpha * render_time + (1.0 - alpha) * averages[name];
			ss << name << " [ms]: " << averages[name] << "\n"; 
		};

		output_pass("rendering_pipeline.json", rendering_pipeline.render_time);

		output_pass(
			"\t" + rendering_pipeline.shadow_mapping.name, 
			rendering_pipeline.shadow_mapping.render_time);

		auto all_passes = rendering_pipeline.shadow_mapping.render_time;
		for (const auto& pass : rendering_pipeline.passes) {
			output_pass("\t" + pass.name, pass.render_time);
			all_passes += pass.render_time;
		}
	
		output_pass("\t(sequencing overhead)", rendering_pipeline.render_time - all_passes);

		text.setString(ss.str());
		text.setCharacterSize(16);

		text.setColor(sf::Color::Black);
		text.setPosition(6, 1);
		window.draw(text);

		text.setColor(sf::Color::White);
		text.setPosition(5, 0);
		window.draw(text);
		window.display();
	}
	else if (!has_written_message) {
		has_written_message = true;
		auto window_size = window.getSize();
		sf::RectangleShape rectangle{sf::Vector2f{window_size}};
		rectangle.setFillColor(sf::Color{0, 0, 0, 180});
		window.draw(rectangle);
		
		text.setString("Error in rendering configuration.");
		text.setCharacterSize(22);
		sf::FloatRect textBounds = text.getLocalBounds();
		text.setPosition(sf::Vector2f(
			floor(window_size.x / 2.0f - (textBounds.left + textBounds.width / 2.0f)), 
			floor(window_size.y / 2.0f - (textBounds.top  + textBounds.height / 2.0f))));
		text.setColor(sf::Color::White);
		window.draw(text);
		window.display();
	}
}

void application::update_file_system_watcher()
{
	fsw.update();

	for (path modified_path : fsw.get_unique_modified_paths(std::chrono::seconds(1))) {
		rendering_assets.reload_model(modified_path);
		rendering_assets.reload_texture(modified_path);
		rendering_pipeline.reload(modified_path, configuration.shader_directory, view);
	}
}

} // namespace cave_demo
