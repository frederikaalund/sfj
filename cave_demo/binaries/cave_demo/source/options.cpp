#include <cave_demo/options.hpp>

#include <iostream>

#include <boost/log/trivial.hpp>
#include <boost/property_tree/ini_parser.hpp>



namespace po = boost::program_options;
using black_label::path;



namespace cave_demo {



////////////////////////////////////////////////////////////////////////////////
/// Window Options
////////////////////////////////////////////////////////////////////////////////
window_options::window_options() noexcept
	: basic_options{"window", true}
	, width{800}
	, height{800}
{
	description.add_options()
		("window.width", po::value<int>(&width)->default_value(width), "Window width.")
		("window.height", po::value<int>(&height)->default_value(height), "Window height.");
}

void window_options::export( ptree& root ) const {
	using namespace std;
	root.put("window.width", to_string(width));
	root.put("window.height", to_string(height));
}


bool window_options::parse_and_validate() {
	// Validate
	const static int min_size{1}, max_size{16384};
	if (min_size > width || max_size < width) throw std::logic_error("Width must be between " + std::to_string(min_size) + " and " + std::to_string(max_size));
	if (min_size > height || max_size < height) throw std::logic_error("Height must be between " + std::to_string(min_size) + " and " + std::to_string(max_size));
}



////////////////////////////////////////////////////////////////////////////////
/// Rendering Options
////////////////////////////////////////////////////////////////////////////////
rendering_options::rendering_options() noexcept
	: basic_options{"rendering"}
{
	description.add_options()
		("rendering.shader_directory", po::value<path>(&shader_directory), "Path to the shader directory. May be relative to the working directory.")
		("rendering.asset_directory", po::value<path>(&asset_directory), "Path to the asset directory. May be relative to the working directory.")
		("rendering.pipeline", po::value<path>(&pipeline)->default_value("default.pipeline.json"), "Path to the rendering pipeline file. May be relative to the shader directory.");
}

void rendering_options::export( ptree& root ) const {
	root.put("rendering.shader_directory", shader_directory);
	root.put("rendering.asset_directory", asset_directory);
	root.put("rendering.pipeline", pipeline);
}


bool rendering_options::parse_and_validate() {
	using namespace black_label;

	// Parse
	shader_directory = canonical_and_preferred(shader_directory);
	asset_directory = canonical_and_preferred(asset_directory);
	pipeline = pipeline.make_preferred();

	// Validate
	if (!is_directory(shader_directory)) throw std::logic_error(shader_directory.string() + " is not a directory.");
	if (!is_directory(asset_directory)) throw std::logic_error(asset_directory.string() + " is not a directory.");
	auto canonical_pipeline = canonical_and_preferred(pipeline, shader_directory);
	if (!is_regular_file(canonical_pipeline)) throw std::logic_error(canonical_pipeline.string() + " is not a file.");
}



////////////////////////////////////////////////////////////////////////////////
/// Options
////////////////////////////////////////////////////////////////////////////////
const black_label::path options::configuration_file_path{"configuration.ini"};

options::options( int argc, const char* argv[] ) noexcept
	: user_requested_help{false}
{
	description
		.add(window.description)
		.add(rendering.description)
		.add_options()("help,h", "Prints this message.");

	variables_map variables_map;

	try {
		parse_command_line(variables_map, argc, argv);
		parse_config_file(variables_map);
		set_variables(variables_map);
		window.complete = rendering.complete = true;
	} catch ( const std::exception& exception ) {
		BOOST_LOG_TRIVIAL(warning) << exception.what();	
	}
}

bool options::reload( const black_label::path& path ) noexcept {
	using namespace std;

	if (configuration_file_path != path) return false;

	BOOST_LOG_TRIVIAL(info) << "Reloading config file";

	variables_map variables_map;
	try {
		auto old_window = window;
		auto old_rendering = rendering;

		parse_config_file(variables_map);
		set_variables(variables_map);
		window.complete = rendering.complete = true;

		if (window.on_reload) window.on_reload(window, move(old_window));
		if (rendering.on_reload) rendering.on_reload(rendering, move(old_rendering));
	} catch (const std::exception& exception) {
		BOOST_LOG_TRIVIAL(warning) << exception.what();
		window.complete = rendering.complete = false;
	}

	return is_complete();
}

void options::export() const {
    basic_options::ptree root;
	window.export(root);
	rendering.export(root);
    write_ini(configuration_file_path.string(), root);
}


void options::parse_command_line( variables_map& variables_map, int argc, const char* argv[] ) {
	store(po::parse_command_line(argc, argv, description), variables_map);
}

void options::parse_config_file( variables_map& variables_map )
{
	if (!exists(configuration_file_path)) return;
	store(po::parse_config_file<char>(configuration_file_path.string().c_str(), description), variables_map);
}

void options::set_variables( variables_map& variables_map ) {
	// Early out if the user has requested help
	if (variables_map.count("help")) {
		user_requested_help = true;
		std::cout << description;
		return;
	}

	// Store the options into the member variables
	notify(variables_map);

	parse_and_validate();
}

} // namespace cave_demo
