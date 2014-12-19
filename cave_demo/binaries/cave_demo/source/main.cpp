#include <cave_demo/initialize_logging.hpp>
#include <cave_demo/options.hpp>
#include <cave_demo/utility.hpp>
#include <cave_demo/window.hpp>

#include <black_label/file_system_watcher.hpp>

#include <boost/range.hpp>
#include <boost/filesystem/fstream.hpp>



using namespace black_label::utility;
using namespace black_label::rendering;
using namespace black_label::file_system_watcher;
using namespace cave_demo;

using namespace std;

using black_label::path;

void strafe_view( view& view ) {
	auto increment = (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
		? 5.0f : 1.0f;

	glm::vec3 strafe{0.0f};

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

	view.strafe(strafe);
}
void pan_view( view& view, int x, int y )
{
	static int last_x, last_y;
	if (!sf::Mouse::isButtonPressed(sf::Mouse::Left)) 
	{
		last_x = x;
		last_y = y;
		return;
	}

	float 
		dx = static_cast<float>(last_x - x),
		dy = static_cast<float>(last_y - y);

	view.pan(dx, dy);

	last_x = x;
	last_y = y;
}
void export_rendering_assets_information( const rendering_assets_type& assets ) {
	boost::filesystem::ofstream stream{"rendering_assets.txt"};

	for (auto static_ : assets.statics | boost::adaptors::map_values) {
		auto asd = std::get<0>(static_);
	}

	stream << "[missing_models]" << "\n";
	for (auto file : boost::make_iterator_range(assets.missing_model_files.unsafe_begin(), assets.missing_model_files.unsafe_end()))
		stream << file << "\n";

	stream << "[models]" << "\n";
	for (auto file : assets.models | boost::adaptors::map_keys) stream << file << "\n";

	stream << "[textures]" << "\n";
	for (auto file : assets.textures | boost::adaptors::map_keys) stream << file << "\n";
}



int main( int argc, char const* argv[] )
{	
	try
	{
		initialize_logging();

		options options{argc, argv};
		if (options.user_requested_help) return EXIT_SUCCESS;
		if (!options.window.complete) return EXIT_FAILURE;

		view view{
			glm::vec3{1200.0f, 200.0f, 200.0f}, 
			glm::vec3{300.0f, 200.0f, -300.0f}, 
			glm::vec3{0.0f, 1.0f, 0.0f}, 
			options.window.width, options.window.height};
		//view view{
		//	glm::vec3{1200.0f, 200.0f, 200.0f}, 
		//	glm::vec3{300.0f, 200.0f, -300.0f}, 
		//	glm::vec3{0.0f, 1.0f, 0.0f}, 
		//	options.window.width, options.window.height,
		//	-100.0f, 100.0f,
		//	-100.0f, 100.0f};
		window window{view.window.x, view.window.y};
		
		black_label::rendering::initialize();

		pipeline rendering_pipeline = (options.rendering.complete)
			? pipeline{&view, options.rendering.shader_directory, options.rendering.pipeline}
			: pipeline{&view};

		gpu::framebuffer framebuffer{black_label::rendering::generate};
		rendering_assets_type rendering_assets{options.rendering.asset_directory};

		auto write_access_file_name = filter::write | filter::access | filter::file_name;
		file_system_watcher file_system_watcher = {
			{boost::filesystem::current_path(), write_access_file_name},
			{options.rendering.asset_directory, write_access_file_name},
			{options.rendering.shader_directory, write_access_file_name}};



////////////////////////////////////////////////////////////////////////////////
/// Callback Registering
////////////////////////////////////////////////////////////////////////////////
		options.window.on_reload = [&] ( window_options& current, window_options former ) {
			if (former == current) return;
			window.resize(options.window.width, options.window.height);
		};

		options.rendering.on_reload = [&] ( rendering_options& current, rendering_options former ) {
			if (former.asset_directory != current.asset_directory) {
				if (boost::filesystem::current_path() != former.asset_directory)
					file_system_watcher.unsubscribe(former.asset_directory);
				file_system_watcher.subscribe(current.asset_directory, write_access_file_name);

				rendering_assets.asset_directory = current.asset_directory;
				rendering_assets.import_missing_models();
			}
			if (former.shader_directory != current.shader_directory) {
				if (boost::filesystem::current_path() != former.shader_directory)
					file_system_watcher.unsubscribe(former.shader_directory);
				file_system_watcher.subscribe(current.shader_directory, write_access_file_name);

				rendering_pipeline.shader_directory = current.shader_directory;
				rendering_pipeline.import(current.pipeline);
			}
			if (former.pipeline != current.pipeline)
				rendering_pipeline.import(current.pipeline);
		};

		window.on_resized = [&] ( int width, int height ) {
			options.window.width = width;
			options.window.height = height;
			options.export();
			view.on_window_resized(width, height);
			rendering_pipeline.on_window_resized(width, height);
		};

		window.on_mouse_moved = [&] ( int x, int y ) { pan_view(view, x, y); };



////////////////////////////////////////////////////////////////////////////////
/// Script
////////////////////////////////////////////////////////////////////////////////
		// This is a placeholder for a script and/or scene file
		set_clear_color({0.9f, 0.934f, 1.0f, 1.0f});
		create_world(rendering_assets);
		


////////////////////////////////////////////////////////////////////////////////
/// Update Loop
////////////////////////////////////////////////////////////////////////////////
		// Returns true until the window is closed
		while (window.update()) {
			if (window.is_focused() && sf::Keyboard::isKeyPressed(sf::Keyboard::R)) {

				// Only strafe the view when the user is actually focusing it
				if (window.is_focused()) strafe_view(view);

				// Render the scene
				rendering_pipeline.render(framebuffer, rendering_assets);
				// Overlay rendering statistics
				draw_statistics(window, options.rendering.asset_directory, rendering_pipeline, options.is_complete());
			}


			// Load models, textures, etc.
			rendering_assets.update();

			//export_rendering_assets_information(rendering_assets);

			// Check for configuration, shader, and asset updates
			file_system_watcher.update();
			for (path modified_path : file_system_watcher.get_unique_modified_paths(std::chrono::seconds(1))) {
				options.reload(modified_path);
				rendering_assets.reload(modified_path);
				rendering_pipeline.reload(modified_path);
			}
		}


	
////////////////////////////////////////////////////////////////////////////////
/// Exit
////////////////////////////////////////////////////////////////////////////////
		return EXIT_SUCCESS;
	}
	catch (const std::exception& exception)
	{
		BOOST_LOG_TRIVIAL(error) << exception.what();
		return EXIT_FAILURE;
	}
}
