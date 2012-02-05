// Version 0.1
#ifndef BLACK_LABEL_WORLD_WORLD_HPP
#define BLACK_LABEL_WORLD_WORLD_HPP

#include "black_label/world/entities.hpp"



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
/// Static Interface
////////////////////////////////////////////////////////////////////////////////
#ifndef BLACK_LABEL_WORLD_DYNAMIC_INTERFACE

	world( configuration configuration );
	
	world& operator =( world const& world );

	entities dynamic_entities;
	entities static_entities;



////////////////////////////////////////////////////////////////////////////////
/// Dynamic Interface for Runtime-loaded Shared Libraries
////////////////////////////////////////////////////////////////////////////////
#else

	struct method_pointers_type
	{
		void* (*construct)( configuration configuration );
		void (*destroy)( void const* world );
		void* (*dynamic_entities)( void* world );
		void* (*static_entities)( void* world );
	} static method_pointers;
		
	world( configuration configuration )
		: _this(method_pointers.construct(configuration))
		, dynamic_entities(method_pointers.dynamic_entities(_this))
		, static_entities(method_pointers.static_entities(_this))
	{}
	~world()
	{ method_pointers.destroy(_this); }

	void* _this;

	entities 
		dynamic_entities,
		static_entities;

#endif
};

} // namespace world
} // namespace black_label



#endif
