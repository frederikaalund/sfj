#include <cave_demo/application.hpp>

#include <tbb/task.h>
#include <tbb/parallel_for.h>

__itt_domain* domain = __itt_domain_create("Task Domain");
__itt_string_handle* UserTask = __itt_string_handle_create("User Task");
__itt_string_handle* UserSubTask = __itt_string_handle_create("UserSubTask");

using namespace black_label::utility;
using namespace black_label::world;
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
	try
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
		environment.create("default_light.fbx", "", make_mat4(1.0f, 500.0, 3500.0f, 600.0f));
		//environment.create("inverted_cube.obj", "", make_mat4(8000.0f, 0.0f, 0.0f, -4000.0f));

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
/// TBB Test
////////////////////////////////////////////////////////////////////////////////
	/*
		__itt_resume();
	
	using namespace tbb;
	typedef std::vector<float> test_type;
	test_type test(1000, 3.14f);

	class my_task : public task
	{
	public:
		test_type& t;
		int w;

		my_task( test_type& t, int w ) : t(t), w(w) {}

		task* execute()
		{
			__itt_task_begin(domain, __itt_null, __itt_null, UserSubTask);
			static const int increment = 100;
			for (int i = 0; increment > i; ++i)
				parallel_for(w, w+increment, [&] ( int v ) {
					for (int j = 0; 100 > j; ++j)
						t[v] *= 10.0;
				});

			w += increment;
			if (w < t.size())
			{
				empty_task& cont = *new(allocate_continuation()) empty_task();

				my_task& ct = *new(cont.allocate_child()) my_task(t, w);
				cont.set_ref_count(1);
				spawn(ct);
			}

			__itt_task_end(domain);
			return nullptr;
		}
	};

	my_task& mt = *new(task::allocate_root()) my_task(test, 0);
	 __itt_task_begin(domain, __itt_null, __itt_null, UserTask);
	task::spawn_root_and_wait(mt);
	 __itt_task_end(domain);
	
	parallel_for(blocked_range<std::vector<float>::size_type>(0, test.size()), [&] ( const blocked_range<std::vector<float>::size_type>& r ) {
		for (auto v = r.begin(); r.end() != v; ++v)
			for (int i = 0; 10 > i; ++i)
				test[v] *= 10.0;
	});

	__itt_pause();
	*/


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
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
}
