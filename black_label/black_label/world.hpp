// Version 0.1
#ifndef BLACK_LABEL_WORLD_WORLD_HPP
#define BLACK_LABEL_WORLD_WORLD_HPP

#include <black_label/world/entities.hpp>
#include <black_label/lsystem.hpp>

#include <string>

#include <glm/glm.hpp>



namespace black_label
{
namespace world
{

class world
{
public:
	typedef size_t id_type;
	typedef glm::mat4 transformation_type;

	typedef entities<std::string, transformation_type, id_type> 
		generic_type;



////////////////////////////////////////////////////////////////////////////////
/// Configuration
////////////////////////////////////////////////////////////////////////////////
	struct configuration
	{
		configuration( 
			generic_type::size_type dynamic_entities_capacity, 
			generic_type::size_type static_entities_capacity )
			: dynamic_entities_capacity(dynamic_entities_capacity)
			, static_entities_capacity(static_entities_capacity)
		{}

		generic_type::size_type 
			dynamic_entities_capacity, 
			static_entities_capacity;
	};



	friend void swap( world& lhs, world& rhs )
	{
		using std::swap;
		swap(lhs.dynamic_entities, rhs.dynamic_entities);
		swap(lhs.static_entities, rhs.static_entities);
	}

	world() {}
	world( configuration configuration )
		: dynamic_entities(configuration.dynamic_entities_capacity)
		, static_entities(configuration.static_entities_capacity)
	{}
	world( world&& other ) { swap(*this, other); }

	world& operator=( world rhs ) { swap(*this, rhs); return *this; }

	generic_type dynamic_entities;
	generic_type static_entities;
};

} // namespace world
} // namespace black_label



#endif
