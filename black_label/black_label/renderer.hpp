// Version 0.1
#ifndef BLACK_LABEL_RENDERER_RENDERER_HPP
#define BLACK_LABEL_RENDERER_RENDERER_HPP

#include <black_label/renderer/camera.hpp>
#include <black_label/world.hpp>
#include <black_label/shared_library/utility.hpp>
#include <black_label/renderer/storage/model.hpp>
#include <black_label/container.hpp>

#include <stdexcept>
#include <memory>

#include <boost/lockfree/fifo.hpp>
#include <boost/noncopyable.hpp>



namespace black_label
{
namespace renderer
{

class BLACK_LABEL_SHARED_LIBRARY renderer
{
public:
	typedef world::world::id_type id_type;
	typedef boost::lockfree::fifo<id_type> dirty_id_container;
	typedef std::unique_ptr<storage::model[]> model_container;



	friend void swap( renderer& lhs, renderer& rhs );

	renderer() {}
	renderer( renderer&& other ) { swap(*this, other); }
	renderer( const world::world* world, camera&& camera );

	renderer& operator=( renderer rhs ) { swap(*this, rhs); return *this; }


	
	bool load_static_model( id_type entity_id );

	template<typename loader_type>
	int load_models( 
		dirty_id_container& ids_to_load,
		container::svector<id_type>& ids_loaded,
		const loader_type& loader );

	void sort_for_rendering( container::svector<id_type>& ids );

	void render_frame();



	const world::world* world;
	camera camera;

	dirty_id_container 
		dirty_static_entities, 
		dirty_dynamic_entities, 
		dirty_lsystems, 
		dirty_turtles;

	model_container static_models, dynamic_models;

	container::svector<id_type> sorted_statics;
	storage::program ubershader, blur_horizontal, blur_vertical, tone_mapper;
	unsigned int framebuffer, main_render, bloom1, bloom2;



protected:
	renderer( const renderer& other ); // Possible, but do you really want to?
};

} // namespace renderer
} // namespace black_label



#endif
