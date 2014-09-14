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

pipeline::pipeline( path shader_directory ) {
	// Program
	shared_ptr<program> shadow_program = add_program(
		program::configuration()
			.vertex_shader(shader_directory / "null.vertex.glsl")
			.fragment_shader(shader_directory / "null.fragment.glsl")
			.preprocessor_commands("#version 330\n"), 
		shader_directory);

	// Pass
	shadow_map_pass = {
		move(shadow_program), 
		{},
		{},
		{},
		GL_DEPTH_BUFFER_BIT,
		0u,
		GL_BACK,
		true,
		true,
		true,
		false,
		{}};
}

bool pipeline::import( path pipeline_file, path shader_directory, std::shared_ptr<camera> camera )
{
	try {	
		ptree ptree;
		read_json(pipeline_file.string(), ptree);
		


////////////////////////////////////////////////////////////////////////////////
/// Textures
////////////////////////////////////////////////////////////////////////////////
		pass::texture_container allocated_textures;

		for (auto child : ptree.get_child("textures"))
		{
			auto texture_ptree = child.second;
			auto name = texture_ptree.get<string>("name");
			auto format_name = texture_ptree.get<string>("format");
			auto filter_name = texture_ptree.get<string>("filter");
			auto wrap_name = texture_ptree.get<string>("wrap");
			auto data = texture_ptree.get("data", "");

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

			auto texture = make_shared<gpu::texture>(target::texture_2d, format, filter, wrap);

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
	
				texture->bind_and_update(random_texture_size, random_texture_size, random_data.data());
			}
			
			allocated_textures.emplace_back(name, texture);
			textures.emplace(name, texture);
		}



////////////////////////////////////////////////////////////////////////////////
/// Buffers
////////////////////////////////////////////////////////////////////////////////
		pass::buffer_container allocated_buffers;

		for (auto child : ptree.get_child("buffers"))
		{
			auto buffer_ptree = child.second;
			auto name = buffer_ptree.get<string>("name");
			auto target_name = buffer_ptree.get<string>("target");
			auto size = buffer_ptree.get<buffer::size_type>("size");

			target::type target;
			if ("shader_storage" == target_name) target = target::shader_storage;
			else throw exception("Unknown target type.");

			auto buffer = make_shared<gpu::buffer>(target, usage::dynamic_copy, size);
			
			allocated_buffers.emplace_back(name, buffer);
			buffers.emplace(name, buffer);
		}



////////////////////////////////////////////////////////////////////////////////
/// Passes
////////////////////////////////////////////////////////////////////////////////
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

			pass::texture_container input_textures;
			if (auto input_texture_children = pass_configuration.get_child_optional("textures.input"))
				for (const auto& input_textures_child : *input_texture_children)
				{
					auto texture_name = input_textures_child.second.get<string>("");
					auto texture = textures.at(texture_name);
					input_textures.emplace_back(move(texture_name), texture.lock());
				}

			pass::texture_container output_textures;
			if (auto output_texture_children = pass_configuration.get_child_optional("textures.output"))
				for (const auto& output_textures_child : *output_texture_children)
				{
					auto texture_name = output_textures_child.second.get<string>("");
					auto texture = textures.at(texture_name);
					output_textures.emplace_back(move(texture_name), texture.lock());
				}

			pass::buffer_container buffers;
			if (auto buffer_children = pass_configuration.get_child_optional("buffers"))
				for (const auto& buffers_child : *buffer_children)
				{
					auto buffer_name = buffers_child.second.get<string>("");
					auto buffer = this->buffers.at(buffer_name);
					buffers.emplace_back(move(buffer_name), buffer.lock());
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

			unsigned int clearing_mask{0u};
			if (auto clear_children = pass_configuration.get_child_optional("clear"))
				for (const auto& clear_child : *clear_children)
				{
					const auto& clear = clear_child.second.get<string>("");
					if ("depth" == clear) clearing_mask |= GL_DEPTH_BUFFER_BIT;
					else if ("color" == clear) clearing_mask |= GL_COLOR_BUFFER_BIT;
					else throw exception("Unknown clear type.");
				}

			unsigned int post_memory_barrier_mask{0u};
			if (auto post_memory_barrier_children = pass_configuration.get_child_optional("memory_barrier.post"))
				for (const auto& post_memory_barrier_child : *post_memory_barrier_children)
				{
					const auto& post_memory_barrier = post_memory_barrier_child.second.get<string>("");
					if ("shader_storage" == post_memory_barrier) post_memory_barrier_mask |= GL_SHADER_STORAGE_BARRIER_BIT;
					else throw exception("Unknown memory_barrier type.");
				}

			unsigned int face_culling_mode{0u};
			if (auto culling_children = pass_configuration.get_child_optional("culling"))
				for (const auto& culling_child : *culling_children)
				{
					const auto& culling = culling_child.second.get<string>("");
					if ("front_faces" == culling) face_culling_mode = (GL_BACK == face_culling_mode) ? GL_FRONT_AND_BACK : GL_FRONT;
					else if ("back_faces" == culling) face_culling_mode = (GL_FRONT == face_culling_mode) ? GL_FRONT_AND_BACK : GL_BACK;
					else throw exception("Unknown culling type.");
				}

			auto test_depth = pass_configuration.get<bool>("test_depth");

			program::configuration configuration;
			configuration.vertex_shader(shader_directory / vertex_program);
			if (!geometry_program.empty()) 
				configuration.geometry_shader(shader_directory / geometry_program);
			configuration.fragment_shader(shader_directory / fragment_program);
			configuration.preprocessor_commands(preprocessor_commands);
			for (const auto& input_textures_value : input_textures)
				configuration.add_vertex_attribute(input_textures_value.first);
			for (const auto& output_textures_value : output_textures)
				configuration.add_fragment_output(output_textures_value.first);

			auto program_ = add_program(configuration, shader_directory);

			passes.emplace_back(
				move(program_), 
				move(input_textures), 
				move(output_textures), 
				move(buffers),
				clearing_mask,
				post_memory_barrier_mask,
				face_culling_mode,
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





bool pipeline::reload_program( path program_file, path shader_directory )
{
	canonical_and_preferred(program_file, shader_directory);

	// Only handle shader files
	if (".glsl" != program_file.extension()) return false;

	// Look up path to find the associated programs
	auto program_range = programs.equal_range(program_file);
	if (program_range.first == program_range.second) return false;

	// Reload each associated program
	for (auto entry = program_range.first; program_range.second != entry; ++entry) {
		auto program = entry->second.lock();
		if (!program) assert(false);
		program->reload(program_file);
	}

	return true;
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