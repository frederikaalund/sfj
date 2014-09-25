#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/renderer.hpp>

#include <GL/glew.h>



using namespace std;



namespace black_label {
namespace renderer {

using namespace gpu;

const path renderer::default_shader_directory;
const path renderer::default_asset_directory;



renderer::renderer( 
	shared_ptr<black_label::renderer::camera> camera_,
	path shader_directory, 
	path asset_directory )
	: shader_directory(shader_directory)
	, assets{asset_directory}
	, camera{camera_}
	, framebuffer{generate}
	, pipeline{shader_directory}
{
	if (GLEW_ARB_texture_storage)
		glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
	glEnable(GL_FRAMEBUFFER_SRGB);
	glClearColor(0.9f, 0.934f, 1.0f, 1.0f);

	//pipeline.import(shader_directory / "rendering_pipeline.json", shader_directory, camera);

	int viewport_data[4];
	glGetIntegerv(GL_VIEWPORT, viewport_data);
	on_window_resized(viewport_data[2], viewport_data[3]);
}

void renderer::render_frame()
{
	assets.update();
	pipeline.render(framebuffer, assets);
}

void renderer::on_window_resized( int width, int height )
{
	camera->on_window_resized(width, height);
	pipeline.on_window_resized(width, height);
}

} // namespace renderer
} // namespace black_label
