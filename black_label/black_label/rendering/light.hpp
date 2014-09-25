
#ifndef BLACK_LABEL_RENDERING_LIGHT_HPP
#define BLACK_LABEL_RENDERING_LIGHT_HPP

#include <black_label/rendering/view.hpp>
#include <black_label/rendering/gpu/texture.hpp>
#include <black_label/utility/serialization/glm.hpp>

#include <cmath>
#include <memory>
#include <string>

#include <boost/serialization/access.hpp>



namespace black_label {
namespace rendering {

enum class light_type {
	point, directional, spot
};

class light
{
public:
	friend class boost::serialization::access;

	friend void swap( light& lhs, light& rhs ) {
		using std::swap;
		swap(lhs.type, rhs.type);
		swap(lhs.position, rhs.position);
		swap(lhs.direction, rhs.direction);
		swap(lhs.color, rhs.color);
		swap(lhs.shadow_map, rhs.shadow_map);
		swap(lhs.view, rhs.view);
	}

	light() {}
	light( 
		light_type type,
		glm::vec3 position, 
		glm::vec3 direction, 
		glm::vec4 color )
		: type{type}
		, position{position}
		, direction{direction}
		, color{color}
	{}
	light( light&& other ) { swap(*this, other); }
	light& operator=( light rhs ) { swap(*this, rhs); return *this; }

	template<typename T>
	static T radius( T cutoff ) { return T{1} / std::sqrt(cutoff); }

	template<typename archive_type>
	void serialize( archive_type& archive, unsigned int version )
	{ archive & type & position & direction & color; }

	light_type type;
	glm::vec3 position, direction;
	glm::vec4 color;
	gpu::storage_texture shadow_map;
	view view;
};

} // namespace rendering
} // namespace black_label



#endif