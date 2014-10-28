#ifndef CAVE_DEMO_OPTIONS_HPP
#define CAVE_DEMO_OPTIONS_HPP

#include <black_label/path.hpp>

#include <functional>

#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>



namespace cave_demo {



////////////////////////////////////////////////////////////////////////////////
/// Basic Options
////////////////////////////////////////////////////////////////////////////////
class basic_options { 
public:
	friend class options;

	basic_options( std::string name, bool complete = false ) noexcept
		: complete{complete}
		, description{std::move(name)}
	{}

	bool complete;


protected:
	using ptree = boost::property_tree::ptree;

	boost::program_options::options_description description;
};



////////////////////////////////////////////////////////////////////////////////
/// Window Options
////////////////////////////////////////////////////////////////////////////////
class window_options : public basic_options {
public:
	window_options() noexcept;

	void export( ptree& root ) const;
	bool parse_and_validate();

	friend bool operator==( const window_options& lhs, const window_options& rhs )
	{ return lhs.width == rhs.width && lhs.height == rhs.height; }

	int width, height;
	std::function<void ( window_options& current, window_options former )> on_reload;
};



////////////////////////////////////////////////////////////////////////////////
/// Rendering Options
////////////////////////////////////////////////////////////////////////////////
class rendering_options : public basic_options {
public:
	rendering_options() noexcept;

	void export( ptree& root ) const;
	bool parse_and_validate();

	friend bool operator==( const rendering_options& lhs, const rendering_options& rhs )
	{
		return lhs.shader_directory == rhs.shader_directory 
			&& lhs.asset_directory == rhs.asset_directory
			&& lhs.pipeline == rhs.pipeline;
	}

	black_label::path shader_directory, asset_directory, pipeline;
	std::function<void ( rendering_options& current, rendering_options former )> on_reload;
};



////////////////////////////////////////////////////////////////////////////////
/// Options
////////////////////////////////////////////////////////////////////////////////
class options {
public:
	static const black_label::path configuration_file_path;

	options( int argc, const char* argv[] ) noexcept;

	bool is_complete() const noexcept { return window.complete && rendering.complete; }
	bool reload( const black_label::path& path ) noexcept;
	void export() const;

	window_options window;
	rendering_options rendering;
	bool user_requested_help;


protected:
	using variables_map = boost::program_options::variables_map;

	void parse_command_line( variables_map& variables_map, int argc, const char* argv[] );
	void parse_config_file( variables_map& variables_map );
	void set_variables( variables_map& variables_map );
	bool parse_and_validate() {
		return window.parse_and_validate()
			&& rendering.parse_and_validate();
	}

	boost::program_options::options_description description;
};

} // namespace cave_demo



#endif
