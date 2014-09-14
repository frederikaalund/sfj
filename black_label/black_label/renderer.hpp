// Version 0.1
#ifndef BLACK_LABEL_RENDERER_RENDERER_HPP
#define BLACK_LABEL_RENDERER_RENDERER_HPP



//#define USE_TILED_SHADING
#define USE_TEXTURE_BUFFER



#include <black_label/container.hpp>
#include <black_label/renderer/assets.hpp>
#include <black_label/renderer/camera.hpp>
#include <black_label/renderer/pipeline.hpp>
#include <black_label/renderer/glew_setup.hpp>
#include <black_label/renderer/gpu/framebuffer.hpp>
#include <black_label/shared_library/utility.hpp>



namespace black_label {
namespace renderer {



class BLACK_LABEL_SHARED_LIBRARY renderer
{
	// Initialize glew as the first thing
	glew_setup glew_setup;

public:
	static const path default_shader_directory;
	static const path default_asset_directory;

	renderer( 
		std::shared_ptr<black_label::renderer::camera> camera_,
		path shader_directory = default_shader_directory, 
		path asset_directory = default_asset_directory );
	renderer( const renderer& ) = delete; // Copying is possible, but do you really want to?

	void optimize_for_rendering( world::group& group );
	void render_frame();
	void reload_asset( path path )
	{
		if (assets.reload_model(path)) return;
		if (assets.reload_texture(path)) return;
		if (pipeline.reload_program(path, shader_directory)) return;
	}
	void on_window_resized( int width, int height );



	path shader_directory;
	assets assets;
	std::shared_ptr<black_label::renderer::camera> camera;

private:
	using light_container = light_grid::light_container;
	
	gpu::framebuffer framebuffer;
	pipeline pipeline;
};

} // namespace renderer
} // namespace black_label



#endif
