#include <cave_demo/utility.hpp>

#include <chrono>
#include <sstream>



using namespace black_label::rendering;
using namespace std;
using namespace std::chrono;



namespace cave_demo {

void draw_statistics(
	window& window,
	const black_label::path& asset_directory,
	const black_label::rendering::pipeline& rendering_pipeline,
	bool options_complete )
{
	static sf::Font font;
	static sf::Text text;
	static bool uninitialized{true};
	if (uninitialized) {
		using namespace black_label;
		system::error_code error_code;
		auto font_file = canonical_and_preferred("fonts/Vegur-Regular.ttf", asset_directory, error_code);

		if (error_code || !font.loadFromFile(font_file.string())) return;

		text.setFont(font);
		uninitialized = false;
	}


	gpu::vertex_array::unbind();
	gpu::buffer::unbind();
	gpu::framebuffer::unbind();
	window.window_.resetGLStates();

	static bool has_written_message{false};
	if (options_complete && rendering_pipeline.is_complete()) {
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
		chrono::high_resolution_clock::duration oit{0};
		for (const auto& pass : rendering_pipeline.passes) {
			if ("oit" == pass.name.substr(0, 3)) {
				oit += pass.render_time;
				continue;
			}
			output_pass("\t" + pass.name, pass.render_time);
			all_passes += pass.render_time;
		}
	
		output_pass("\toit (all passes)", oit);

		output_pass("\t(sequencing overhead)", rendering_pipeline.render_time - all_passes);

		text.setString(ss.str());
		text.setCharacterSize(16);

		text.setColor(sf::Color::Black);
		text.setPosition(6, 1);
		window.window_.draw(text);

		text.setColor(sf::Color::White);
		text.setPosition(5, 0);
		window.window_.draw(text);
		window.window_.display();
	}
	else if (!has_written_message) {
		has_written_message = true;
		auto window_size = window.window_.getSize();
		sf::RectangleShape rectangle{sf::Vector2f{window_size}};
		rectangle.setFillColor(sf::Color{0, 0, 0, 180});
		window.window_.draw(rectangle);
		
		text.setString("Error in rendering config.");
		text.setCharacterSize(22);
		sf::FloatRect textBounds = text.getLocalBounds();
		text.setPosition(sf::Vector2f(
			floor(window_size.x / 2.0f - (textBounds.left + textBounds.width / 2.0f)), 
			floor(window_size.y / 2.0f - (textBounds.top  + textBounds.height / 2.0f))));
		text.setColor(sf::Color::White);
		window.window_.draw(text);
		window.window_.display();
	}
}

} // namespace cave_demo
