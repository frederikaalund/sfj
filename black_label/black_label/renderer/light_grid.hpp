#ifndef BLACK_LABEL_RENDERER_LIGHT_GRID_HPP
#define BLACK_LABEL_RENDERER_LIGHT_GRID_HPP



#include <black_label/container/darray.hpp>
#include <black_label/container/svector.hpp>
#include <black_label/renderer/camera.hpp>
#include <black_label/renderer/light.hpp>
#include <black_label/renderer/program.hpp>
#include <black_label/renderer/storage/gpu/texture.hpp>
#include <black_label/shared_library/utility.hpp>

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>



namespace black_label {
namespace renderer {

class light_grid
{
public:
	typedef container::svector<light> light_container;
	typedef container::darray<std::vector<float> > index_grid_type;
	struct grid_data { float offset, count; };
	typedef container::darray<grid_data> grid_type;
	typedef std::vector<float> index_list_type;



	light_grid( int tile_size, black_label::renderer::camera& camera, const light_container& lights );

	void on_window_resized( int width, int height );
	void update();
	void use( const core_program& program, int& texture_unit ) const;

	int tile_size() const { return tile_size_; }
	int tiles_x() const { return tiles_x_; }
	int tiles_y() const { return tiles_y_; }

	

protected:
	light_grid( const light_grid& other ); // Possible, but do you really want to?



private:
	int tile_size_, tiles_x_, tiles_y_;

	index_grid_type index_grid;
	grid_type grid;
	index_list_type index_list;

	storage::gpu::texture_buffer<float> gpu_index_list;
	storage::gpu::texture_buffer<glm::vec2> gpu_grid;

	black_label::renderer::camera& camera;
	const light_container& lights;
};

} // namespace renderer
} // namespace black_label



#endif
