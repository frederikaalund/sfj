#include <cave_demo/application.hpp>



using namespace black_label::world;
using namespace black_label::thread_pool;
using namespace cave_demo;



glm::mat4 make_mat4( float scale, float x, float y, float z )
{
	auto m = glm::mat4(); m[3][0] = x; m[3][1] = y; m[3][2] = z;
	return std::move(m);
}



int main( int argc, char const* argv[] )
{	
	application demo(argc, argv);
	


////////////////////////////////////////////////////////////////////////////////
/// World Test
////////////////////////////////////////////////////////////////////////////////
	world::entities_type::group environment(demo.world.static_entities);

	environment.create("scene_2.dae", glm::mat4());
	environment.create("scene_2.dae", make_mat4(1.0f, 0.0f, 0.0f, -10.0f));
	environment.create("sphere.nff", make_mat4(1.0f, 0.0f, 0.0f, -500.0f));
	environment.create("inverted_cube.obj", make_mat4(5000.0f, 0.0f, 0.0f, -2500.0f));

	demo.renderer.report_dirty_models(
		demo.world.models.cbegin(),
		demo.world.models.cend());
	
	demo.renderer.report_dirty_static_entities(
		environment.cbegin(),
		environment.cend());
		


	for (auto entity = environment.cebegin(); environment.ceend() != entity; ++entity)
	{
		auto& test1 = entity.model();
		auto& test2 = entity.transformation();
		auto test3 = entity.id();
		auto test4 = *entity;
	}
	


////////////////////////////////////////////////////////////////////////////////
/// Loop
////////////////////////////////////////////////////////////////////////////////
	while (demo.window_is_open())
		demo.update();


	
////////////////////////////////////////////////////////////////////////////////
/// Exit
////////////////////////////////////////////////////////////////////////////////
	return 0;
}
