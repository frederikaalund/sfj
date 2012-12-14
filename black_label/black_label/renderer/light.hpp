#ifndef BLACK_LABEL_RENDERER_LIGHT_HPP
#define BLACK_LABEL_RENDERER_LIGHT_HPP

#include <black_label/utility/serialization/glm.hpp>

#include <algorithm>

#include <boost/serialization/access.hpp>



namespace black_label {
namespace renderer {

class light
{
public:
	friend class boost::serialization::access;

	light() {}
	light( 
		glm::vec3 position, 
		glm::vec4 color, 
		float constant_attenuation = 1.0f, 
		float linear_attenuation = 0.0f,
		float quadratic_attenuation = 0.0f )
		: position(position)
		, color(color)
		, constant_attenuation(constant_attenuation)
		, linear_attenuation(linear_attenuation)
		, quadratic_attenuation(quadratic_attenuation)
	{}

	float radius( float cutoff ) const
	{
		return 0.5f * (sqrt(-cutoff * (-cutoff * linear_attenuation*linear_attenuation + 4.0f * cutoff * quadratic_attenuation * constant_attenuation - 4.0f * quadratic_attenuation)) - cutoff * linear_attenuation) / (cutoff * quadratic_attenuation);
	}

	template<typename archive_type>
	void serialize( archive_type& archive, unsigned int version )
	{
		archive & position & color & constant_attenuation & linear_attenuation
			& quadratic_attenuation;
	}

	glm::vec3 position;
	glm::vec4 color;
	float constant_attenuation, linear_attenuation, quadratic_attenuation;
};

} // namespace renderer
} // namespace black_label



#endif