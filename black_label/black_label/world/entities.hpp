#ifndef BLACK_LABEL_WORLD_ENTITIES_HPP
#define BLACK_LABEL_WORLD_ENTITIES_HPP

#ifdef BLACK_LABEL_WORLD_ENTITIES_IMPLEMENTATION
#include <boost/atomic.hpp> 
#endif



namespace black_label
{
namespace world
{

struct entities
{

	struct matrix4f { float data[4*4]; };

	typedef size_t size_type;



////////////////////////////////////////////////////////////////////////////////
/// Static Interface
////////////////////////////////////////////////////////////////////////////////
#ifndef BLACK_LABEL_WORLD_DYNAMIC_INTERFACE

	entities( size_type capacity );
	~entities();

	size_type create( matrix4f transformation );
	void remove( size_type entity );

	size_type capacity, size;
	matrix4f* transformations;

	size_type* ids;


	
////////////////////////////////////////////////////////////////////////////////
/// Dynamic Interface for Runtime-loaded Shared Libraries
////////////////////////////////////////////////////////////////////////////////
#else

	struct method_pointers_type 
	{
		size_type (*create)( 
			void* entities,
			matrix4f transformation );
		void (*remove)( 
			void* entities,
			size_type id );
	} static method_pointers;

	entities( void* entities )
		: _this(entities)
		, owns_this(false)
	{}
	~entities()
	{ /* if (owns_this) _destroy_ ; */ }
	size_type create( matrix4f transformation )
	{ return method_pointers.create(_this, transformation); }
	void remove( size_type entity )
	{ return method_pointers.remove(_this, entity); }

	void* _this;
	bool owns_this;

#endif
};

} // namespace world
} // namespace black_label



#endif
