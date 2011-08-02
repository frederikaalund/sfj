#include "black_label/world/entities.hpp"

#include "black_label/shared_library/utility.hpp"



namespace black_label
{
namespace world
{

entities::entities( size_type capacity )
	: capacity(capacity)
	, size(0)
	, transformations(new matrix4f[capacity])
	, ids(new size_type[capacity])
{
	for (size_type id = 0; id < capacity; ++id) ids[id] = id;
}

entities::~entities()
{
	delete[] transformations;
	delete[] ids;
}



////////////////////////////////////////////////////////////////////////////////
/// Shared Library Runtime Interface
////////////////////////////////////////////////////////////////////////////////
BLACK_LABEL_SHARED_LIBRARY entities::size_type add_entity( 
	entities& entities, 
	entities::matrix4f transformation )
{
	entities::size_type id = entities.ids[entities.size++];

	entities.transformations[id] = transformation;

	return id;
}

BLACK_LABEL_SHARED_LIBRARY void remove_entity( 
	entities& entities, 
	entities::size_type id )
{
	entities.ids[--entities.size] = id;
}

} // namespace world
} // namespace black_label
