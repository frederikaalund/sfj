#include <cave_demo/utility.hpp>

#include <black_label/path.hpp>
#include <black_label/world/entities.hpp>

#include <chrono>
#include <sstream>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/range/adaptors.hpp>



using namespace black_label::rendering;
using namespace black_label::world;
using namespace std;
using namespace std::chrono;

using black_label::path;



namespace cave_demo {

glm::mat4 make_mat4( float scale, float x = 0.0f, float y = 0.0f, float z = 0.0f )
{
	auto m = glm::mat4(); 
	m[3][0] = x; m[3][1] = y; m[3][2] = z;
	m[0][0] = m[1][1] = m[2][2] = scale;
	return m;
}
void create_world( rendering_assets_type& rendering_assets ) {
	static group all_statics;
	
	path scene_file{"scene.json"};
	BOOST_LOG_TRIVIAL(info) << "Importing scene file " << scene_file << "...";

	boost::property_tree::ptree root;
	boost::property_tree::read_json(scene_file.string(), root);

	vector<path> models, dynamics;
	vector<glm::mat4> transformations;

	for (auto entity : root.get_child("entities") | boost::adaptors::map_values) {
		auto model = entity.get<string>("model");
		models.emplace_back(move(model));

		auto dynamic = entity.get<string>("dynamic");
		dynamics.emplace_back(move(dynamic));

		glm::mat4 transformation; // Identity matrix
		if (auto transformation_root_ = entity.get_child_optional("transformation"))
			for (auto& transformation_child : *transformation_root_)
			{
				if ("translate" == transformation_child.first) {

				}
			}
		transformations.emplace_back(transformation);
	}
	
	all_statics.emplace_back(std::make_shared<entities>(
		models,
		dynamics,
		//transformations
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
}

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
		chrono::high_resolution_clock::duration ldm{0};
		for (const auto& pass : rendering_pipeline.passes) {
			all_passes += pass.render_time;
			if ("ldm" == pass.name.substr(0, 3)) {
				ldm += pass.render_time;
				continue;
			}
			output_pass("\t" + pass.name, pass.render_time);
		}
	
		output_pass("\tldm (all passes)", ldm);

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
