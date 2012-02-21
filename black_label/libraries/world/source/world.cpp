#include "black_label/world.hpp"

#include "black_label/shared_library/utility.hpp"



namespace black_label
{
namespace world
{

world::world( configuration configuration )
	: dynamic_entities(configuration.dynamic_entities_capacity)
	, static_entities(configuration.static_entities_capacity)
	
{

}
    
world::~world()
{
    
}



////////////////////////////////////////////////////////////////////////////////
/// Dynamic Interface for Runtime-loaded Shared Library
////////////////////////////////////////////////////////////////////////////////
BLACK_LABEL_SHARED_LIBRARY world* world_construct(
	world::configuration configuration )
{
	return new world(configuration);
}

BLACK_LABEL_SHARED_LIBRARY void world_destroy(
	world const* world )
{
	delete world;
}

BLACK_LABEL_SHARED_LIBRARY entities* world_dynamic_entities(
	world* world )
{
	return &world->dynamic_entities;
}

BLACK_LABEL_SHARED_LIBRARY entities* world_static_entities(
	world* world )
{
	return &world->static_entities;
}

} // namespace world
} // namespace black_label
