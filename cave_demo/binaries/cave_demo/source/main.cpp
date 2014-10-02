#include <cave_demo/application.hpp>

#include <iterator>

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

using namespace std;



glm::mat4 make_mat4( float scale, float x = 0.0f, float y = 0.0f, float z = 0.0f )
{
	auto m = glm::mat4(); 
	m[3][0] = x; m[3][1] = y; m[3][2] = z;
	m[0][0] = m[1][1] = m[2][2] = scale;
	return m;
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

		using rendering_entities = application::rendering_assets_type::external_entities;

		vector<rendering_entities> asset_data;
		asset_data.reserve(demo.all_statics.size());
		int id{0};
		for (const auto& entity : demo.all_statics)
			asset_data.emplace_back(
				id++, 
				boost::make_iterator_range(entity->begin(entity->models), entity->end(entity->models)),
				boost::make_iterator_range(entity->begin(entity->transformations), entity->end(entity->transformations)));

		demo.rendering_assets.add_statics(cbegin(asset_data), cend(asset_data));
		
		//later test1(5000, true, [&] {
		//	BOOST_LOG_TRIVIAL(info) << "Adding all statics";
		//	demo.rendering_assets.add_statics(cbegin(asset_data), cend(asset_data));
		//	later test2(1000, true, [&] {
		//		BOOST_LOG_TRIVIAL(info) << "Removing all statics";
		//		demo.rendering_assets.remove_statics(cbegin(asset_data), cend(asset_data));
		//		later test3(7500, true, [&] {
		//			BOOST_LOG_TRIVIAL(info) << "Adding all statics";
		//			demo.rendering_assets.add_statics(cbegin(asset_data), cend(asset_data));
		//			later test4(7500, true, [&] {
		//				BOOST_LOG_TRIVIAL(info) << "Removing all statics";
		//				demo.rendering_assets.remove_statics(cbegin(asset_data), cend(asset_data));
		//			});
		//		});
		//	});
		//});
		


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
		BOOST_LOG_TRIVIAL(info) << "Exception: " << e.what();
		return 1;
	}
}
