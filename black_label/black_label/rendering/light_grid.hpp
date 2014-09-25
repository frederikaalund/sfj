#ifndef BLACK_LABEL_RENDERING_LIGHT_GRID_HPP
#define BLACK_LABEL_RENDERING_LIGHT_GRID_HPP



#include <black_label/rendering/view.hpp>
#include <black_label/rendering/light.hpp>
#include <black_label/rendering/program.hpp>
#include <black_label/rendering/gpu/texture.hpp>
#include <black_label/shared_library/utility.hpp>

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>



namespace black_label {
namespace rendering {

class light_grid
{
public:
	typedef std::vector<light> light_container;
	typedef std::vector<std::vector<int>> index_grid_type;
	struct grid_data { int offset, count; };
	typedef std::vector<grid_data> grid_type;
	typedef std::vector<int> index_list_type;



	light_grid( int tile_size, black_label::rendering::view& view, const light_container& lights );

	void on_window_resized( int width, int height );
	void update();
	void use( const core_program& program, unsigned int& texture_unit ) const;

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

	gpu::texture_buffer gpu_index_list;
	gpu::texture_buffer gpu_grid;

	black_label::rendering::view& view;
	const light_container& lights;
};

} // namespace rendering
} // namespace black_label



#endif
