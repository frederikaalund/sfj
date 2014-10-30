#include <cave_demo/initialize_logging.hpp>
#include <cave_demo/options.hpp>
#include <cave_demo/utility.hpp>
#include <cave_demo/window.hpp>

#include <black_label/file_system_watcher.hpp>
#include <black_label/rendering.hpp>
#include <black_label/world/entities.hpp>

#include <boost/range.hpp>
#include <boost/filesystem/fstream.hpp>



using namespace black_label::utility;
using namespace black_label::utility;
using namespace black_label::world;
using namespace black_label::rendering;
using namespace black_label::file_system_watcher;
using namespace cave_demo;

using namespace std;

using black_label::path;
using rendering_assets_type = assets<
	int, 
	black_label::world::entities::model_type*, 
	black_label::world::entities::transformation_type*>;



glm::mat4 make_mat4( float scale, float x = 0.0f, float y = 0.0f, float z = 0.0f )
{
	auto m = glm::mat4(); 
	m[3][0] = x; m[3][1] = y; m[3][2] = z;
	m[0][0] = m[1][1] = m[2][2] = scale;
	return m;
}
void create_world( rendering_assets_type& rendering_assets ) {
	static group all_statics;

	all_statics.emplace_back(std::make_shared<entities>(
		std::initializer_list<entities::model_type>{"models/crytek-sponza/sponza.fbx", "models/teapot/teapot.obj", "models/references/sphere_rough.fbx"},
		std::initializer_list<entities::dynamic_type>{"sponza.bullet", "cube.bullet", "house.bullet"},
		std::initializer_list<entities::transformation_type>{make_mat4(1.0f), make_mat4(1.0f), make_mat4(1.0f, 0.0f, 200.0f, 0.0f)}));

	using rendering_entities = rendering_assets_type::external_entities;

	vector<rendering_entities> asset_data;
	asset_data.reserve(all_statics.size());
	int id{0};
	for (const auto& entity : all_statics)
		asset_data.emplace_back(
			id++, 
			boost::make_iterator_range(entity->begin(entity->models), entity->end(entity->models)),
			boost::make_iterator_range(entity->begin(entity->transformations), entity->end(entity->transformations)));

	rendering_assets.add_statics(cbegin(asset_data), cend(asset_data));
		
	//later test1(5000, true, [&] {
	//	BOOST_LOG_TRIVIAL(info) << "Adding all statics";
	//	rendering_assets.add_statics(cbegin(asset_data), cend(asset_data));
	//	later test2(1000, true, [&] {
	//		BOOST_LOG_TRIVIAL(info) << "Removing all statics";
	//		rendering_assets.remove_statics(cbegin(asset_data), cend(asset_data));
	//		later test3(7500, true, [&] {
	//			BOOST_LOG_TRIVIAL(info) << "Adding all statics";
	//			rendering_assets.add_statics(cbegin(asset_data), cend(asset_data));
	//			later test4(7500, true, [&] {
	//				BOOST_LOG_TRIVIAL(info) << "Removing all statics";
	//				rendering_assets.remove_statics(cbegin(asset_data), cend(asset_data));
	//			});
	//		});
	//	});
	//});
}
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
			// Only strafe the view when the user is actually focusing it
			if (window.is_focused()) strafe_view(view);

			// Load models, textures, etc.
			rendering_assets.update();
			// Render the scene
			rendering_pipeline.render(framebuffer, rendering_assets);
			// Overlay rendering statistics
			draw_statistics(window, options.rendering.asset_directory, rendering_pipeline, options.is_complete());

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
