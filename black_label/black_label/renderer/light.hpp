#ifndef BLACK_LABEL_RENDERER_LIGHT_HPP
#define BLACK_LABEL_RENDERER_LIGHT_HPP

#include <glm/glm.hpp>



namespace black_label
{
namespace renderer
{

class light
{
public:
	light() {}
	light( glm::vec3 position, float radius, glm::vec4 color )
		: position(position), radius(radius), color(color) {}

	glm::vec3 position;
	float radius;
	glm::vec4 color;
};

} // namespace renderer
} // namespace black_label



#endif