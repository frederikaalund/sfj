// Version 0.1
#ifndef BLACK_LABEL_WORLD_WORLD_HPP
#define BLACK_LABEL_WORLD_WORLD_HPP

#include <black_label/world/entities.hpp>

#include <string>

#include <glm/glm.hpp>



namespace black_label {
namespace world {

class world
{
public:
	using model_type = std::string;
	using dynamic_type = std::string;
	using transformation_type = glm::mat4;

	using entities_type = entities<model_type, dynamic_type, transformation_type>;
	using entity_id_type = entities_type::size_type;



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



	world() {}
	world( configuration configuration )
		: dynamic_entities(configuration.dynamic_entities_capacity)
		, static_entities(configuration.static_entities_capacity)
	{}
	world( const world& other ) = delete;
	world( world&& other ) = default;
	world& operator=( world rhs ) { std::swap(*this, rhs); return *this; }

	entities_type dynamic_entities;
	entities_type static_entities;
};

} // namespace world
} // namespace black_label



#endif
