#ifndef BLACK_LABEL_RENDERING_PIPELINE_HPP
#define BLACK_LABEL_RENDERING_PIPELINE_HPP

#include <black_label/rendering/light.hpp>
#include <black_label/rendering/pass.hpp>
#include <black_label/utility/threading_building_blocks/path.hpp>

#include <unordered_map>

#include <boost/algorithm/cxx11/all_of.hpp>
#include <boost/range/adaptor/filtered.hpp>

#include <tbb/concurrent_unordered_map.h>



namespace black_label {
namespace rendering {



class pipeline
{
public:
	using pass_container = std::vector<pass>;
	using program_container = tbb::concurrent_unordered_multimap<path, std::weak_ptr<program>>;
	using texture_container = std::unordered_map<std::string, std::weak_ptr<gpu::storage_texture>>;
	using buffer_container = std::unordered_map<std::string, std::weak_ptr<gpu::buffer>>;
	using index_bound_buffer_container = std::unordered_map<std::string, std::weak_ptr<gpu::index_bound_buffer>>;
	using reset_container = std::vector<std::pair<std::weak_ptr<gpu::buffer>, const std::vector<std::uint8_t>>>;

	friend void swap( pipeline& lhs, pipeline& rhs )
	{
		using std::swap;
		swap(lhs.complete, rhs.complete);
		swap(lhs.programs, rhs.programs);
		swap(lhs.textures, rhs.textures);
		swap(lhs.buffers, rhs.buffers);
		swap(lhs.index_bound_buffers, rhs.index_bound_buffers);
		swap(lhs.buffers_to_reset_pre_first_frame, rhs.buffers_to_reset_pre_first_frame);
		swap(lhs.passes, rhs.passes);
		swap(lhs.shadow_mapping, rhs.shadow_mapping);
	}

	pipeline( path shader_directory );
	pipeline( path pipeline_file, path shader_directory, const view& view )
		: pipeline{shader_directory}
	{ import(pipeline_file, shader_directory, view); }
	pipeline( pipeline&& other ) : pipeline{""} { swap(*this, other); };
	pipeline& operator=( pipeline rhs ) { swap(*this, rhs); return *this; }

	bool is_complete() const {
		return complete && boost::algorithm::all_of(programs | boost::adaptors::map_values, 
			[] ( const auto& program ) { return program.lock()->is_complete(); });
	}
	bool import( path pipeline_file, path shader_directory, const view& view );
	bool reload( path path_, path shader_directory, const view& view ) {
		canonical_and_preferred(path_, shader_directory);

		// Program files
		if (".glsl" == path_.extension())
			return reload_program(path_);
		// Program files
		if (".json" == path_.extension())
			return import(path_, shader_directory, view);;

		return false;
	}

	template<typename assets_type>
	void render_shadow_maps( gpu::framebuffer& framebuffer, const assets_type& assets ) const {
		for (const light& light : assets.shadow_casting_lights)
			shadow_mapping.render(
				framebuffer, 
				assets, 
				light.view, 
				utility::make_range(light.shadow_map));
	}

	template<typename assets_type>
	//void render_passes( gpu::framebuffer& framebuffer, const assets_type& assets ) const
	void render_passes( gpu::framebuffer& framebuffer, const assets_type& assets )
	//{ for (const auto& pass : passes) pass.render(framebuffer, assets); }
	{ 
		for (auto& pass : passes) { 
			if ("oit" == pass.name) {
				if (!boost::empty(assets.shadow_casting_lights))
				{
					pass.view = &assets.shadow_casting_lights[0].get().view;
				}
			}

			pass.render(framebuffer, assets);
		}
	}
			
	template<typename assets_type>
	//void render( gpu::framebuffer& framebuffer, const assets_type& assets ) const {
	void render( gpu::framebuffer& framebuffer, const assets_type& assets ) {
		if (!is_complete()) return;
		auto start_time = std::chrono::high_resolution_clock::now();
		reset(buffers_to_reset_pre_first_frame);
		render_shadow_maps(framebuffer, assets);
		render_passes(framebuffer, assets);
		pass::wait_for_opengl();
		render_time = std::chrono::high_resolution_clock::now() - start_time;
	}

	template<typename range>
	void reset( const range& range ) const {
		for (auto& entry : range) {
			auto& buffer = entry.first;
			const auto& data = entry.second;

			auto locked_buffer = buffer.lock();
			if (!locked_buffer) continue;

			locked_buffer->bind_and_update(data.size(), data.data());
		}
	}

	void on_window_resized( int width, int height ) {
		for (auto& texture : textures) {
			if ("random" == texture.first) continue;

			auto locked_texture = texture.second.lock();
			if (!locked_texture) continue;

			*locked_texture = gpu::storage_texture(
				*locked_texture,
				width,
				height);
		}
	}



	bool complete;
	program_container programs;
	texture_container textures;
	buffer_container buffers;
	index_bound_buffer_container index_bound_buffers;
	reset_container buffers_to_reset_pre_first_frame;
	pass_container passes;
	basic_pass shadow_mapping;
	mutable std::chrono::high_resolution_clock::duration render_time;



private:
	bool reload_program( path program_file );
	std::shared_ptr<program> add_program( program::configuration configuration, path shader_directory );
};



} // namespace rendering
} // namespace black_label



#endif
