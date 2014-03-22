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
	using buffer_container = std::unordered_map<std::string, std::weak_ptr<gpu::texture>>;

	friend void swap( pipeline& lhs, pipeline& rhs )
	{
		using std::swap;
		swap(lhs.programs, rhs.programs);
		swap(lhs.buffers, rhs.buffers);
		swap(lhs.shadow_map_passes, rhs.shadow_map_passes);
		swap(lhs.passes, rhs.passes);
	}

	pipeline() {}
	pipeline( path pipeline_file, path shader_directory, std::shared_ptr<camera> camera )
	{ import(pipeline_file, shader_directory, camera); }
	pipeline( const pipeline& ) = delete;
	pipeline( pipeline&& other ) : pipeline{} { swap(*this, other); };
	pipeline& operator=( pipeline rhs ) { swap(*this, rhs); return *this; }

	bool import( path pipeline_file, path shader_directory, std::shared_ptr<camera> camera );

	void add_shadow_map_pass( light light, path shader_directory );

	program_container programs;
	buffer_container buffers;
	pass_container shadow_map_passes, passes;

private:
	std::shared_ptr<program> add_program( program::configuration configuration, path shader_directory );
};



} // namespace renderer
} // namespace black_label



#endif
