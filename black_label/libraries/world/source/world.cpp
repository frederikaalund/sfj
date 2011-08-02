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

world& world::operator =( world const& world )
{
	

	return *this;
}



////////////////////////////////////////////////////////////////////////////////
/// Shared Library Runtime Interface
////////////////////////////////////////////////////////////////////////////////
BLACK_LABEL_SHARED_LIBRARY world* construct_world(
	world::configuration configuration )
{
	return new world(configuration);
}

BLACK_LABEL_SHARED_LIBRARY void destroy_world(
	world const* world )
{
	delete world;
}

BLACK_LABEL_SHARED_LIBRARY void copy_world(
	world& destination,
	world const& source )
{
	destination = source;
}

} // namespace world
} // namespace black_label
