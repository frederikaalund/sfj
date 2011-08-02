// Version 0.1
#ifndef BLACK_LABEL_WORLD_WORLD_HPP
#define BLACK_LABEL_WORLD_WORLD_HPP

#include "world/entities.hpp"



namespace black_label
{
namespace world
{

struct world
{

////////////////////////////////////////////////////////////////////////////////
/// Configuration
////////////////////////////////////////////////////////////////////////////////
	struct configuration
	{
		configuration( 
			entities::size_type dynamic_entities_capacity, 
			entities::size_type static_entities_capacity )
			: dynamic_entities_capacity(dynamic_entities_capacity)
			, static_entities_capacity(static_entities_capacity)
		{

		}

		entities::size_type 
			dynamic_entities_capacity, 
			static_entities_capacity;
	};



////////////////////////////////////////////////////////////////////////////////
/// World
////////////////////////////////////////////////////////////////////////////////
	world( configuration configuration );
	
	world& operator =( world const& world );

	entities dynamic_entities;
	entities static_entities;



////////////////////////////////////////////////////////////////////////////////
/// Shared Library Runtime Interface
////////////////////////////////////////////////////////////////////////////////
	typedef world* construct_world_type(
		configuration configuration );
	typedef void destroy_world_type(
		world const* world );
	typedef void copy_world_type(
		world& destination,
		world const& source );

	static construct_world_type* construct;
	static destroy_world_type* destroy;
	static copy_world_type* copy;
};

} // namespace world
} // namespace black_label



#endif
