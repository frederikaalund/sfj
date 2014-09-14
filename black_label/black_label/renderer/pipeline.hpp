#ifndef BLACK_LABEL_RENDERER_PIPELINE_HPP
#define BLACK_LABEL_RENDERER_PIPELINE_HPP



#include <black_label/renderer/light.hpp>
#include <black_label/renderer/pass.hpp>
#include <black_label/utility/threading_building_blocks/path.hpp>

#include <unordered_map>

#include <tbb/concurrent_unordered_map.h>



namespace black_label {
namespace renderer {



class pipeline
{
public:
	using pass_container = std::vector<pass>;
	using program_container = tbb::concurrent_unordered_multimap<path, std::weak_ptr<program>>;
	using texture_container = std::unordered_map<std::string, std::weak_ptr<gpu::texture>>;
	using buffer_container = std::unordered_map<std::string, std::weak_ptr<gpu::buffer>>;

	friend void swap( pipeline& lhs, pipeline& rhs )
	{
		using std::swap;
		swap(lhs.programs, rhs.programs);
		swap(lhs.textures, rhs.textures);
		swap(lhs.buffers, rhs.buffers);
		swap(lhs.passes, rhs.passes);
		swap(lhs.shadow_map_pass, rhs.shadow_map_pass);
	}

	pipeline( path shader_directory );
	pipeline( path pipeline_file, path shader_directory, std::shared_ptr<camera> camera )
	{ import(pipeline_file, shader_directory, camera); }
	pipeline( const pipeline& ) = delete;
	pipeline( pipeline&& other ) : pipeline{""} { swap(*this, other); };
	pipeline& operator=( pipeline rhs ) { swap(*this, rhs); return *this; }

	bool import( path pipeline_file, path shader_directory, std::shared_ptr<camera> camera );
	bool reload_program( path program_file, path shader_directory );
	void render(
		const gpu::framebuffer& framebuffer, 
		const assets& assets ) // TODO: const
	{		
		// Don't render if a program is incomplete (didn't compile or link)
		for (auto& program : programs)
			if (!program.second.lock()->is_complete())
				return;

		for (const auto& light : assets.lights) {
			if (!light.shadow_map) continue;
			shadow_map_pass.output_textures.clear();
			shadow_map_pass.output_textures.emplace_back("", light.shadow_map);
			shadow_map_pass.camera = light.camera;
			shadow_map_pass.render(framebuffer, assets);
		}
		for (const auto& pass : passes) pass.render(framebuffer, assets);
	}

	program_container programs;
	texture_container textures;
	buffer_container buffers;
	pass_container passes;
	pass shadow_map_pass;



private:
	std::shared_ptr<program> add_program( program::configuration configuration, path shader_directory );
	void reload_program( path program_file, program& program );
};



} // namespace renderer
} // namespace black_label



#endif
