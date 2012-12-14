#ifndef BLACK_LABEL_RENDERER_MATERIAL_HPP
#define BLACK_LABEL_RENDERER_MATERIAL_HPP

#include <black_label/shared_library/utility.hpp>
#include <black_label/utility/serialization/glm.hpp>

#include <string>

#include <boost/serialization/access.hpp>

#include <glm/gtc/type_ptr.hpp>



namespace black_label {
namespace renderer {

struct material 
{
	material( bool enabled = true ) : enabled(enabled) {}

	operator bool() const { return enabled; }

	template<typename archive_type>
	void serialize( archive_type& archive, unsigned int version )
	{
		archive & enabled & ambient & diffuse & specular & emissive & alpha & shininess 
			& ambient_texture & diffuse_texture & specular_texture 
			& height_texture;
	}

	

	bool enabled;
	glm::vec3 ambient, diffuse, specular, emissive;
	float alpha, shininess;
	std::string ambient_texture, diffuse_texture, specular_texture, 
		height_texture;
};

} // namespace renderer
} // namespace black_label



#endif