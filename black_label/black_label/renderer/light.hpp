#ifndef BLACK_LABEL_RENDERER_LIGHT_HPP
#define BLACK_LABEL_RENDERER_LIGHT_HPP

#include <black_label/renderer/camera.hpp>
#include <black_label/utility/serialization/glm.hpp>

#include <cmath>
#include <memory>
#include <string>

#include <boost/serialization/access.hpp>



namespace black_label {
namespace renderer {

enum class light_type {
	point, directional, spot
};

class light
{
public:
	friend class boost::serialization::access;

	light() {}
	light( 
		light_type type,
		glm::vec3 position, 
		glm::vec3 direction, 
		glm::vec4 color,
		std::string shadow_map = std::string{} )
		: type{type}
		, position{position}
		, direction{direction}
		, color{color}
		, shadow_map{std::move(shadow_map)}
	{}

	template<typename T>
	static T radius( T cutoff ) { return T{1} / std::sqrt(cutoff); }

	template<typename archive_type>
	void serialize( archive_type& archive, unsigned int version )
	{ archive & type & position & direction & color; }

	light_type type;
	glm::vec3 position, direction;
	glm::vec4 color;
	std::string shadow_map;
	std::shared_ptr<camera> camera;
};

} // namespace renderer
} // namespace black_label



#endif