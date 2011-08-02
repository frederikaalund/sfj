#ifndef BLACK_LABEL_WORLD_ENTITIES_HPP
#define BLACK_LABEL_WORLD_ENTITIES_HPP

namespace black_label
{
namespace world
{

struct entities
{
	struct matrix4f { float data[4*4]; };

	typedef size_t size_type;

	entities( size_type capacity );
	~entities();

	size_type capacity, size;
	matrix4f* transformations;

	size_type* ids;



////////////////////////////////////////////////////////////////////////////////
/// Shared Library Runtime Interface
////////////////////////////////////////////////////////////////////////////////
	typedef entities::size_type add_entity_type(
		entities&,
		entities::matrix4f transformation );
	typedef void remove_entity_type(
		entities&,
		entities::size_type index );

	static add_entity_type* add;
	static remove_entity_type* remove;
};

} // namespace world
} // namespace black_label



#endif
