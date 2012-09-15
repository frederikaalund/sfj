#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/renderer/light_grid.hpp>

#include <boost/math/constants/constants.hpp>

#include <glm/glm.hpp>

#include <GL/glew.h>



namespace black_label
{
namespace renderer
{

light_grid::light_grid( 
	int tile_size, 
	black_label::renderer::camera& camera,
	const light_container& lights ) 
	: tile_size_(tile_size)
	, camera(camera)
	, lights(lights)
{
	glGenBuffers(1, &lights_buffer);
	glGenTextures(1, &lights_texture);

	glGenBuffers(1, &index_list_buffer);
	glGenTextures(1, &index_list_texture);

	glGenBuffers(1, &grid_buffer);
	glGenTextures(1, &grid_texture);
}

light_grid::~light_grid()
{
	if (!index_list_buffer) return;

	glDeleteBuffers(1, &lights_buffer);
	glDeleteTextures(1, &lights_texture);

	glDeleteBuffers(1, &index_list_buffer);
	glDeleteTextures(1, &index_list_texture);

	glDeleteBuffers(1, &grid_buffer);
	glDeleteTextures(1, &grid_texture);
}



void light_grid::on_window_resized( int width, int height )
{
	tiles_x_ = (width + tile_size_ - 1) / tile_size_;
	tiles_y_ = (height + tile_size_ - 1) / tile_size_;
	auto tile_count = tiles_x_ * tiles_y_;
	index_grid = index_grid_type(tile_count);
	grid = grid_type(tile_count);
}



void light_grid::update()
{
	float
		alpha = camera.fovy * boost::math::constants::pi<float>() / 360.0f,
    tan_alpha = tan(alpha);



	std::for_each(index_grid.begin(), index_grid.end(), [] ( std::vector<int>& list ) { list.clear(); });

	for (light_container::size_type i = 0; lights.size() > i; ++i)
	{
		auto& light = lights[i];

		auto lower_left = glm::vec2(0.0f, 0.0f);
		auto upper_right = glm::vec2(camera.window_f.x, camera.window_f.y);

		int solution_count = 4;

		auto L = camera.view_matrix * glm::vec4(light.position, 1.0f);
		auto L_s = L*L;
		auto r = light.radius;
		auto r_s = r*r;
		glm::vec3 N, P;

		float half_near_plane_height = camera.z_near * tan_alpha;
		float half_near_plane_width = half_near_plane_height * camera.aspect_ratio;
		
		// For x
		auto D_4 = (r_s * L_s.x - (L_s.x + L_s.z) * (r_s - L_s.z));

		if (0.0f < D_4)
		{
			auto test1 = [&] () {
				if (0.0f == N.x) { --solution_count; return; }

				N.z = (r - N.x * L.x) / L.z;

				P.z = (L_s.x + L_s.z - r_s) / (L.z - (N.z / N.x) * L.x);
				P.y = L.y;
				P.x = -(P.z * N.z) / N.x;

				if (0.0f <= P.z) { --solution_count; return; }

				float 
					x_ndc = (N.z * this->camera.z_near) / (N.x * half_near_plane_width),
					x_wc = (x_ndc + 1.0f) * this->camera.window_f.x * 0.5f;

				if (P.x < L.x && lower_left.x < x_wc)
					lower_left.x = x_wc;

				if (P.x > L.x && upper_right.x > x_wc)
					upper_right.x = x_wc;		
			};

			N.x = (r * L.x + sqrt(D_4)) / (L_s.x + L_s.z);
			test1();

			N.x = (r * L.x - sqrt(D_4)) / (L_s.x + L_s.z);
			test1();
		}
		/*
		else
			solution_count -= 2;*/

		// Now for y
		D_4 = r_s * L_s.y - (L_s.y + L_s.z) * (r_s - L_s.z);

		if (0.0f < D_4)
		{
			auto test1 = [&] () {
				if (0.0f == N.y) { --solution_count; return; }

				N.z = (r - N.y * L.y) / L.z;
				
				P.z = (L_s.y + L_s.z - r_s) / (L.z - (N.z / N.y) * L.y);
				P.y = -(P.z * N.z) / N.y;
				P.x = L.x;

				if (0.0f <= P.z) { --solution_count; return; }
				
				float 
					y_ndc = (N.z * this->camera.z_near) / (N.y * half_near_plane_height),
					y_wc = (y_ndc + 1.0f) * this->camera.window_f.y * 0.5f;

				if (P.y < L.y && lower_left.y < y_wc)
					lower_left.y = y_wc;

				if (P.y > L.y && upper_right.y > y_wc)
					upper_right.y = y_wc;
			};

			N.y = (r * L.y + sqrt(D_4)) / (L_s.y + L_s.z);
			test1();

			N.y = (r * L.y - sqrt(D_4)) / (L_s.y + L_s.z);
			test1();
		}
		/*
		else
			solution_count -= 2;*/
		
		if (0 == solution_count) continue;

		for (float x = lower_left.x, x_tc = x / tile_size_; upper_right.x + tile_size_ > x && x_tc < tiles_x_; x += tile_size_, x_tc = x / tile_size_)
			for (float y = lower_left.y, y_tc = y / tile_size_; upper_right.y + tile_size_ > y && y_tc < tiles_y_; y += tile_size_, y_tc = y / tile_size_)
				index_grid[tiles_x_ * static_cast<int>(y_tc) + static_cast<int>(x_tc)].push_back(i);
	}
	

	index_list.clear();
	auto index_grid_it = index_grid.begin();
	auto grid_it = grid.begin();
	int offset = 0;
	for (; index_grid.end() != index_grid_it; ++index_grid_it, ++grid_it)
	{
		grid_it->offset = offset;
		grid_it->count = index_grid_it->size();
		offset += index_grid_it->size();

		index_list.insert(index_list.end(), index_grid_it->cbegin(), index_grid_it->cend());
	}

	

	if (!lights.empty())
	{
		glBindBuffer(GL_TEXTURE_BUFFER, lights_buffer);
		glBufferData(GL_TEXTURE_BUFFER, lights.size() * sizeof(light), lights.data(), GL_STREAM_DRAW);
	}

	if (!index_list.empty())
	{
		glBindBuffer(GL_TEXTURE_BUFFER, index_list_buffer);
		glBufferData(GL_TEXTURE_BUFFER, index_list.size() * sizeof(int), index_list.data(), GL_STREAM_DRAW);
	}

	glBindBuffer(GL_TEXTURE_BUFFER, grid_buffer);
	glBufferData(GL_TEXTURE_BUFFER, grid.capacity() * sizeof(grid_data), grid.data(), GL_STREAM_DRAW);
}

void light_grid::bind( program::id_type program_id ) const
{
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_BUFFER, lights_texture);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, lights_buffer);
	glUniform1i(glGetUniformLocation(program_id, "lights"), 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_BUFFER, index_list_texture);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_R32I, index_list_buffer);
	glUniform1i(glGetUniformLocation(program_id, "light_index_list"), 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_BUFFER, grid_texture);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32I, grid_buffer);
	glUniform1i(glGetUniformLocation(program_id, "light_grid"), 3);
}

} // namespace renderer
} // namespace black_label
