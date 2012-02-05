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



entities::size_type entities::create( matrix4f transformation )
{
	entities::size_type entity = ids[size++];

	transformations[entity] = transformation;

	return entity;
}

void entities::remove( size_type entity )
{
	ids[--size] = entity;
}



////////////////////////////////////////////////////////////////////////////////
/// Dynamic Interface for Runtime-loaded Shared Library
////////////////////////////////////////////////////////////////////////////////
BLACK_LABEL_SHARED_LIBRARY entities::size_type entities_create( 
	entities* entities, 
	entities::matrix4f transformation )
{
	return entities->create(transformation);
}

BLACK_LABEL_SHARED_LIBRARY void entities_remove( 
	entities* entities, 
	entities::size_type entity )
{
	entities->remove(entity);
}

} // namespace world
} // namespace black_label
