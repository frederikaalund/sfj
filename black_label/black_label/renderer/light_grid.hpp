#ifndef BLACK_LABEL_RENDERER_LIGHT_GRID_HPP
#define BLACK_LABEL_RENDERER_LIGHT_GRID_HPP



#define USE_TEXTURE_BUFFER



#include <black_label/container/darray.hpp>
#include <black_label/container/svector.hpp>
#include <black_label/renderer/camera.hpp>
#include <black_label/renderer/light.hpp>
#include <black_label/renderer/program.hpp>
#include <black_label/shared_library/utility.hpp>

#include <cstdint>
#include <vector>



namespace black_label
{
namespace renderer
{

class light_grid
{
public:
	typedef container::svector<light> light_container;
	typedef container::darray<std::vector<int> > index_grid_type;
	struct grid_data { int offset, count; };
	typedef container::darray<grid_data> grid_type;
	typedef std::vector<float> index_list_type;



	light_grid( int tile_size, black_label::renderer::camera& camera, const light_container& lights );
	~light_grid();

	void on_window_resized( int width, int height );
	void update();
	void bind( program::id_type program_id ) const;

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

	unsigned int index_list_buffer, index_list_texture, 
		grid_buffer, grid_texture;

	black_label::renderer::camera& camera;
	const light_container& lights;
};

} // namespace renderer
} // namespace black_label



#endif
