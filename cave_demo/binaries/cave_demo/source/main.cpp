#include <cave_demo/application.hpp>

#include <tbb/task.h>
#include <tbb/parallel_for.h>

#include <boost/log/trivial.hpp>

#include <functional>
#include <chrono>
#include <future>
#include <cstdio>

class later
{
public:
    template <class callable, class... arguments>
    later(int after, bool async, callable&& f, arguments&&... args)
    {
        std::function<typename std::result_of<callable(arguments...)>::type()> task(std::bind(std::forward<callable>(f), std::forward<arguments>(args)...));

        if (async)
        {
            std::thread([after, task]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(after));
                task();
            }).detach();
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(after));
            task();
        }
    }

};


//__itt_domain* domain = __itt_domain_create("Task Domain");
//__itt_string_handle* UserTask = __itt_string_handle_create("User Task");
//__itt_string_handle* UserSubTask = __itt_string_handle_create("UserSubTask");

using namespace black_label::utility;
using namespace black_label::world;
using namespace cave_demo;



glm::mat4 make_mat4( float scale, float x = 0.0f, float y = 0.0f, float z = 0.0f )
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
		application demo{argc, argv};

	

////////////////////////////////////////////////////////////////////////////////
/// World Test
////////////////////////////////////////////////////////////////////////////////

		demo.all_statics.emplace_back(std::make_shared<entities>(
			std::initializer_list<entities::model_type>{"models/crytek-sponza/sponza.fbx", "models/teapot/teapot.obj", "models/references/sphere_rough.fbx"},
			std::initializer_list<entities::dynamic_type>{"sponza.bullet", "cube.bullet", "house.bullet"},
			std::initializer_list<entities::transformation_type>{make_mat4(1.0f), make_mat4(1.0f), make_mat4(1.0f, 0.0f, 200.0f, 0.0f)}));

		demo.renderer.optimize_for_rendering(demo.all_statics);
		
		later test1(5000, true, [&] {
			BOOST_LOG_TRIVIAL(info) << "Adding all statics";
			demo.renderer.assets.add_statics(const_group_cast(demo.all_statics));
			later test2(7500, true, [&] {
				BOOST_LOG_TRIVIAL(info) << "Removing all statics";
				demo.renderer.assets.remove_statics(const_group_cast(demo.all_statics));
				later test3(7500, true, [&] {
					BOOST_LOG_TRIVIAL(info) << "Adding all statics";
					demo.renderer.assets.add_statics(const_group_cast(demo.all_statics));
					later test4(7500, true, [&] {
						BOOST_LOG_TRIVIAL(info) << "Removing all statics";
						demo.renderer.assets.remove_statics(const_group_cast(demo.all_statics));
					});
				});
			});
		});

		//world::entities_type::group environment(demo.world.static_entities);
		//world::entities_type::group doodads(demo.world.dynamic_entities);

		////auto scene_2 = environment.create("scene_2.fbx", "scene_2.bullet");
		////environment.create("scene_2.dae", make_mat4(1.0f, 0.0f, 0.0f, -10.0f));
		//auto sponza = environment.create("sponza.obj", "sponza.bullet");
		//environment.create("default_light.fbx", "", make_mat4(1.0f, 500.0, 3500.0f, 600.0f));
		////environment.create("inverted_cube.obj", "", make_mat4(8000.0f, 0.0f, 0.0f, -4000.0f));

		//for (int i = 0; i < 10; ++i)
		//{
		//	auto sphere = doodads.create("dynamica_test_1.fbx", "dynamica_test_1.bullet", make_mat4(10.0f, 10.0f * rand() / RAND_MAX, 10.0f + i * 20.0f, 10.0f * rand() / RAND_MAX));
		//}
		
		//demo.renderer.report_dirty_models(
		//	demo.world.models.cbegin(),
		//	demo.world.models.cend());
	
		//demo.renderer.report_dirty_static_entities(
		//	environment.cbegin(),
		//	environment.cend());

		//demo.renderer.report_dirty_dynamic_entities(
		//	doodads.cbegin(),
		//	doodads.cend());



////////////////////////////////////////////////////////////////////////////////
/// Loop
////////////////////////////////////////////////////////////////////////////////
		while (demo.window_is_open()) demo.update();


	
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
