#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/rendering/pipeline.hpp>

#include <random>

#include <boost/log/trivial.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <GL/glew.h>



using namespace black_label::rendering::gpu;

using namespace std;

using namespace boost::property_tree;



namespace black_label {
namespace rendering {

pipeline::pipeline( path shader_directory )
	: complete{false}
	, shadow_mapping{
		"shadow_mapping",
		add_program(program::configuration()
			.vertex_shader(shader_directory / "null.vertex.glsl")
			.fragment_shader(shader_directory / "null.fragment.glsl")
			.preprocessor_commands("#version 330\n"), 
			shader_directory),
		GL_DEPTH_BUFFER_BIT,
		GL_BACK,
		render_mode{}
			.set(render_mode::statics)
			.set(render_mode::dynamics)
			.set(render_mode::test_depth)}
{}

bool pipeline::import( path pipeline_file, path shader_directory, const view& view )
{
	programs.clear();
	textures.clear();
	buffers.clear();
	index_bound_buffers.clear();
	buffers_to_reset_pre_first_frame.clear();
	passes.clear();

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
			auto data = texture_ptree.get_optional<string>("data");

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

			auto width = texture_ptree.get<float>("width", 1.0);
			auto height = texture_ptree.get<float>("height", 1.0);

			auto texture = make_shared<gpu::storage_texture>(target::texture_2d, format, filter, wrap, width, height);

			if (data) {
				if ("random" == *data) {
					mt19937 random_number_generator;
					uniform_real_distribution<float> distribution(-1.0f, 1.0f);
					int random_texture_size = 800;
					auto random_data = vector<glm::vec3>(random_texture_size * random_texture_size);
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
				} else throw exception("Unknown data type.");
			}
			
			allocated_textures.emplace_back(name, texture);
			textures.emplace(name, texture);
		}



////////////////////////////////////////////////////////////////////////////////
/// Buffers
////////////////////////////////////////////////////////////////////////////////
		pass::buffer_container allocated_buffers;
		pass::index_bound_buffer_container allocated_index_bound_buffers;

		for (auto child : ptree.get_child("buffers"))
		{
			auto buffer_ptree = child.second;
			auto name = buffer_ptree.get<string>("name");
			auto target_name = buffer_ptree.get<string>("target");
			auto size = buffer_ptree.get<buffer::size_type>("size");
			auto binding = buffer_ptree.get_optional<buffer::size_type>("binding");
			auto data_name = buffer_ptree.get_optional<string>("data");
			auto reset_name = buffer_ptree.get_optional<string>("reset");

			target::type target;
			if ("shader_storage" == target_name) target = target::shader_storage;
			else if ("atomic_counter" == target_name) target = target::atomic_counter;
			else throw exception("Unknown target type.");

			vector<uint8_t> data;
			if (data_name) {
				if ("null" == *data_name)
					data.resize(size);
				else throw exception("Unknown data type.");
			}

			shared_ptr<gpu::buffer> buffer;
			if (binding) {
				auto index_bound_buffer = make_shared<gpu::index_bound_buffer>(target, usage::dynamic_copy, *binding, size, (data.empty()) ? nullptr : data.data());
				allocated_index_bound_buffers.emplace_back(name, index_bound_buffer);
				index_bound_buffers.emplace(name, index_bound_buffer);
				buffer = index_bound_buffer;
			} else {
				buffer = make_shared<gpu::buffer>(target, usage::dynamic_copy, size, (data.empty()) ? nullptr : data.data());
				allocated_buffers.emplace_back(name, buffer);
				buffers.emplace(name, buffer);
			}

			if (reset_name) {
				if (data.empty()) throw exception("Reset is specified without data.");
				if ("pre_first_pass" == *reset_name)
					buffers_to_reset_pre_first_frame.emplace_back(move(buffer), move(data));
				else throw exception("Unknown reset type.");
			}
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

			pass::buffer_container pass_buffers;
			pass::index_bound_buffer_container pass_index_bound_buffers;
			if (auto buffer_children = pass_configuration.get_child_optional("buffers"))
				for (const auto& buffers_child : *buffer_children)
				{
					auto buffer_name = buffers_child.second.get<string>("");

					// First search through the regular buffers.
					auto buffer = buffers.find(buffer_name);
					if (buffers.cend() != buffer)
						pass_buffers.emplace_back(move(buffer_name), buffer->second.lock());
					// Otherwise, use the index-bound buffers.
					else
						pass_index_bound_buffers.emplace_back(move(buffer_name), index_bound_buffers.at(buffer_name).lock());					
				}

			render_mode render_mode;
			for (const auto& model_child : pass_configuration.get_child("models"))
			{
				const auto& model_value = model_child.second.get<string>("");
				if ("statics" == model_value) render_mode.set(render_mode::statics);
				else if ("dynamics" == model_value) render_mode.set(render_mode::dynamics);
				else if ("screen_aligned_quad" == model_value) render_mode.set(render_mode::screen_aligned_quad);
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

			render_mode.set(render_mode::test_depth, pass_configuration.get<bool>("test_depth", false));
			render_mode.set(render_mode::materials, pass_configuration.get<bool>("materials", true));

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
				move(name),
				move(program_), 
				move(input_textures), 
				move(output_textures), 
				move(pass_buffers),
				move(pass_index_bound_buffers),
				clearing_mask,
				post_memory_barrier_mask,
				face_culling_mode,
				render_mode,
				&view);
		}
	} 
	catch (exception exception)
	{
		BOOST_LOG_TRIVIAL(error) << "Error reading " << pipeline_file << ": " << exception.what(); 
		return complete = false;
	}

	on_window_resized(view.window.x, view.window.y);
	return complete = true;
}

bool pipeline::reload_program( path program_file )
{
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

std::shared_ptr<program> pipeline::add_program(program::configuration configuration, path shader_directory)
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

	return program_;
}



} // namespace rendering
} // namespace black_label