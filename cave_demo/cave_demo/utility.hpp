#ifndef CAVE_DEMO_SETUP_HPP
#define CAVE_DEMO_SETUP_HPP

#include <cave_demo/window.hpp>

#include <black_label/path.hpp>
#include <black_label/rendering.hpp>
#include <black_label/rendering/pipeline.hpp>
#include <black_label/rendering/view.hpp>



namespace cave_demo {

using rendering_assets_type = black_label::rendering::assets<
	int,
	black_label::world::entities::model_type*,
	black_label::world::entities::transformation_type*>;

void create_world( rendering_assets_type& rendering_assets, black_label::rendering::view& view );

void draw_statistics(
	window& window,
	const black_label::path& asset_directory, 
	const black_label::rendering::pipeline& rendering_pipeline,
	bool options_complete );

} // namespace cave_demo



#include <functional>
#include <chrono>
#include <future>
#include <cstdio>

namespace cave_demo {

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

} // namespace cave_demo



#endif
