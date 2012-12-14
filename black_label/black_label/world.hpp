// Version 0.1
#ifndef BLACK_LABEL_WORLD_WORLD_HPP
#define BLACK_LABEL_WORLD_WORLD_HPP

#include <black_label/container/svector.hpp>
#include <black_label/world/entities.hpp>

#include <string>

#include <glm/glm.hpp>



namespace black_label {
namespace world {

class world
{
public:
	typedef std::string model_type;
	typedef std::string dynamics_type;
	typedef glm::mat4 transformation_type;

	typedef container::svector<model_type> model_container;
	typedef model_container::size_type model_id_type;

	typedef entities<model_type, dynamics_type, transformation_type> entities_type;
	typedef entities_type::size_type entity_id_type;



////////////////////////////////////////////////////////////////////////////////
/// Configuration
////////////////////////////////////////////////////////////////////////////////
	struct configuration
	{
		configuration() {}
		configuration( 
			entities_type::size_type dynamic_entities_capacity, 
			entities_type::size_type static_entities_capacity )
			: dynamic_entities_capacity(dynamic_entities_capacity)
			, static_entities_capacity(static_entities_capacity)
		{}

		entities_type::size_type 
			dynamic_entities_capacity, 
			static_entities_capacity;
	};



	friend void swap( world& lhs, world& rhs )
	{
		using std::swap;
		swap(lhs.models, rhs.models);
		swap(lhs.dynamic_entities, rhs.dynamic_entities);
		swap(lhs.static_entities, rhs.static_entities);
	}

	world() 
		: dynamic_entities(models)
		, static_entities(models)
	{}
	world( configuration configuration )
		: models(configuration.dynamic_entities_capacity 
			+ configuration.static_entities_capacity)
		, dynamic_entities(models, configuration.dynamic_entities_capacity)
		, static_entities(models, configuration.static_entities_capacity)
	{}
	world( world&& other ) 
		: dynamic_entities(models)
		, static_entities(models)
	{ swap(*this, other); }

	world& operator=( world rhs ) { swap(*this, rhs); return *this; }

	model_container models;
	entities_type dynamic_entities;
	entities_type static_entities;
};

} // namespace world
} // namespace black_label



#endif
