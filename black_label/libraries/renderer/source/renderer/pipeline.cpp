#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/renderer/pipeline.hpp>

#include <random>

#include <boost/log/trivial.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <GL/glew.h>



using namespace black_label::renderer::gpu;

using namespace std;

using namespace boost::property_tree;



namespace black_label {
namespace renderer {

bool pipeline::import( path pipeline_file, path shader_directory, std::shared_ptr<camera> camera )
{
	try {	
		ptree ptree;
		read_json(pipeline_file.string(), ptree);

		pass::buffer_container allocated_buffers;

		for (auto child : ptree.get_child("buffers"))
		{
			auto buffer_ptree = child.second;
			auto name = buffer_ptree.get<string>("name");
			auto format_name = buffer_ptree.get<string>("format");
			auto filter_name = buffer_ptree.get<string>("filter");
			auto wrap_name = buffer_ptree.get<string>("wrap");
			auto data = buffer_ptree.get("data", "");

			format::type format;
			if ("rgba8" == format_name) format = format::rgba8;
			else if ("rgba16f" == format_name) format = format::rgba16f;
			else if ("rgba32f" == format_name) format = format::rgba32f;
			else if ("depth16" == format_name) format = format::depth16;
			else if ("depth24" == format_name) format = format::depth24;
			else if ("depth32f" == format_name) format = format::depth32f;
			else throw exception("Unknown format type.");

			filter::type filter;
			if ("nearest" == filter_name) filter = filter::nearest;
			else if ("linear" == filter_name) filter = filter::linear;
			else throw exception("Unknown filter type.");

			wrap::type wrap;
			if ("clamp_to_edge" == wrap_name) wrap = wrap::clamp_to_edge;
			else if ("repeat" == wrap_name) wrap = wrap::repeat;
			else if ("mirrored_repeat" == wrap_name) wrap = wrap::mirrored_repeat;
			else throw exception("Unknown wrap type.");

			auto buffer = make_shared<texture>(target::texture_2d, format, filter, wrap);

			if ("random" == data)
			{
				mt19937 random_number_generator;
				uniform_real_distribution<float> distribution(-1.0f, 1.0f);
				int random_texture_size = 800;
				auto random_data = container::darray<glm::vec3>(random_texture_size * random_texture_size);
				std::generate(random_data.begin(), random_data.end(), [&] () -> glm::vec3 { 
					glm::vec3 v;
					do 
					{
						v.x = distribution(random_number_generator);
						v.y = distribution(random_number_generator);
						v.z = distribution(random_number_generator);
					} while (glm::length(v) > 1.0f);
					return v * 0.5f + glm::vec3(0.5f);
				});
	
				buffer->bind_and_update(random_texture_size, random_texture_size, random_data.data());
			}
			
			allocated_buffers.emplace_back(name, buffer);
			buffers.emplace(name, buffer);
		}

		for (auto pass_child : ptree.get_child("passes"))
		{
			auto pass_configuration = pass_child.second;
			auto name = pass_configuration.get<string>("name");
			auto vertex_program = pass_configuration.get<string>("vertex_program");
			auto geometry_program = pass_configuration.get("geometry_program", "");
			auto fragment_program = pass_configuration.get<string>("fragment_program");

			string preprocessor_commands;
			for (const auto& preprocessor_command_child : pass_configuration.get_child("preprocessor_commands"))
			{
				const auto& preprocessor_command = preprocessor_command_child.second.get<string>("");
				preprocessor_commands += preprocessor_command + "\n";
			}		

			pass::buffer_container input;
			for (const auto& input_child : pass_configuration.get_child("buffers.input"))
			{
				auto buffer_name = input_child.second.get<string>("");
				auto buffer = buffers.at(buffer_name);
				input.emplace_back(move(buffer_name), buffer.lock());
			}

			pass::buffer_container output;
			for (const auto& output_child : pass_configuration.get_child("buffers.output"))
			{
				auto buffer_name = output_child.second.get<string>("");
				auto buffer = buffers.at(buffer_name);
				output.emplace_back(move(buffer_name), buffer.lock());
			}

			bool render_statics = false, 
				render_dynamics = false, 
				render_screen_aligned_quad = false;
			for (const auto& model_child : pass_configuration.get_child("models"))
			{
				const auto& model_value = model_child.second.get<string>("");
				if ("statics" == model_value) render_statics = true;
				else if ("dynamics" == model_value) render_dynamics = true;
				else if ("screen_aligned_quad" == model_value) render_screen_aligned_quad = true;
				else throw exception("Invalid model type.");
			}

			int clearing_mask{0};
			for (const auto& clear_child : pass_configuration.get_child("clear"))
			{
				const auto& clear = clear_child.second.get<string>("");
				if ("depth" == clear) clearing_mask |= GL_DEPTH_BUFFER_BIT;
				else if ("color" == clear) clearing_mask |= GL_COLOR_BUFFER_BIT;
				else throw exception("Unknown clear type.");
			}

			auto test_depth = pass_configuration.get<bool>("test_depth");

			program::configuration configuration;
			configuration.vertex_shader(shader_directory / vertex_program);
			if (!geometry_program.empty()) 
				configuration.geometry_shader(shader_directory / geometry_program);
			configuration.fragment_shader(shader_directory / fragment_program);
			configuration.preprocessor_commands(preprocessor_commands);
			for (const auto& input_value : input)
				configuration.add_vertex_attribute(input_value.first);
			for (const auto& output_value : output)
				configuration.add_fragment_output(output_value.first);

			auto program_ = add_program(configuration, shader_directory);

			passes.emplace_back(
				move(program_), 
				move(input), 
				move(output), 
				clearing_mask, 
				test_depth, 
				render_statics, 
				render_dynamics, 
				render_screen_aligned_quad,
				camera);
		}
	} 
	catch (exception exception)
	{
		BOOST_LOG_TRIVIAL(error) << "Error reading " << pipeline_file << ": " << exception.what(); 
		return false;
	}

	return true;
}

void pipeline::add_shadow_map_pass( light light, path shader_directory )
{
	// Buffer
	auto shadow_map = make_shared<texture>(target::texture_2d, format::depth32f, filter::nearest, wrap::clamp_to_edge);
	//shadow_map->bind();
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	shadow_map->bind_and_update(light.camera->window.x, light.camera->window.y);
	buffers.emplace(light.shadow_map, shadow_map);
					
	// Program
	shared_ptr<program> program_;
	if (shadow_map_passes.empty())
	{
		program_ = add_program(
			program::configuration()
				.vertex_shader(shader_directory / "null.vertex.glsl")
				.fragment_shader(shader_directory / "null.fragment.glsl")
				.preprocessor_commands("#version 330\n"), 
			shader_directory);
	}
	else
		program_ = shadow_map_passes.front().program_;

	// Pass
	pass shadow_map_pass{
		move(program_), 
		pass::buffer_container{},
		{{light.shadow_map, shadow_map}},
		GL_DEPTH_BUFFER_BIT,
		true,
		true,
		true,
		false,
		light.camera};

	shadow_map_passes.push_back(move(shadow_map_pass));
}

std::shared_ptr<program> pipeline::add_program( program::configuration configuration, path shader_directory )
{
	auto program_ = make_shared<program>(configuration.shader_directory(shader_directory));
		
	if (!configuration.path_to_vertex_shader_.empty())
		programs.insert({configuration.path_to_vertex_shader_, program_});
	if (!configuration.path_to_geometry_shader_.empty())
		programs.insert({configuration.path_to_geometry_shader_, program_});
	if (!configuration.path_to_fragment_shader_.empty())
		programs.insert({configuration.path_to_fragment_shader_, program_});

	auto info_log = program_->get_aggregated_info_log();
	if (!info_log.empty()) BOOST_LOG_TRIVIAL(error) << info_log;

	return move(program_);
}



} // namespace renderer
} // namespace black_label