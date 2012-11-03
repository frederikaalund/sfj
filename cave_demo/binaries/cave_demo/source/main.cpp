#include <cave_demo/application.hpp>



using namespace black_label::utility;
using namespace black_label::world;
using namespace black_label::thread_pool;
using namespace cave_demo;



glm::mat4 make_mat4( float scale, float x, float y, float z )
{
	auto m = glm::mat4(); 
	m[3][0] = x; m[3][1] = y; m[3][2] = z;
	m[0][0] = m[1][1] = m[2][2] = scale;
	return std::move(m);
}



int main( int argc, char const* argv[] )
{	
	application demo(argc, argv);
	


////////////////////////////////////////////////////////////////////////////////
/// World Test
////////////////////////////////////////////////////////////////////////////////
	world::entities_type::group environment(demo.world.static_entities);
	world::entities_type::group doodads(demo.world.dynamic_entities);

	//auto scene_2 = environment.create("scene_2.fbx", "scene_2.bullet");
	//environment.create("scene_2.dae", make_mat4(1.0f, 0.0f, 0.0f, -10.0f));
	auto sponza = environment.create("sponza.obj", "sponza.bullet");
	environment.create("default_light.fbx", "", make_mat4(1.0f, 0.0f, 10.0f, 0.0f));
	environment.create("inverted_cube.obj", "", make_mat4(5000.0f, 0.0f, 0.0f, -2500.0f));

	for (int i = 0; i < 10; ++i)
	{
		auto sphere = doodads.create("dynamica_test_1.fbx", "dynamica_test_1.bullet", make_mat4(10.0f, 10.0f * rand() / RAND_MAX, 10.0f + i * 20.0f, 10.0f * rand() / RAND_MAX));
		demo.dynamics.report_dirty_dynamic_dynamics(*sphere);
	}

	demo.renderer.report_dirty_models(
		demo.world.models.cbegin(),
		demo.world.models.cend());
	
	demo.renderer.report_dirty_static_entities(
		environment.cbegin(),
		environment.cend());

	demo.renderer.report_dirty_dynamic_entities(
		doodads.cbegin(),
		doodads.cend());

	demo.dynamics.report_dirty_static_dynamics(*sponza);
	



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
