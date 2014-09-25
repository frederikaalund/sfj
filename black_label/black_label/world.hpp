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

	using entities_type = entities_base<model_type, dynamic_type, transformation_type>;
	using entity_id_type = entities_type::size_type;

	friend void swap( world& lhs, world& rhs )
	{
		using std::swap;
		swap(lhs.dynamic_entities, rhs.dynamic_entities);
		swap(lhs.dynamic_entities, rhs.dynamic_entities);
	}

	world() {}
	world( const world& other ) = delete;
	world( world&& other ) = default;
	world& operator=( world rhs ) { swap(*this, rhs); return *this; }

	entities_type dynamic_entities, static_entities;
};

} // namespace world
} // namespace black_label



#endif
