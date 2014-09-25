#ifndef BLACK_LABEL_RENDERING_MATERIAL_HPP
#define BLACK_LABEL_RENDERING_MATERIAL_HPP

#include <black_label/rendering/argument/material.hpp>
#include <black_label/utility/serialization/glm.hpp>
#include <black_label/utility/serialization/path.hpp>

#include <string>
#include <utility>

#include <boost/serialization/access.hpp>

#include <glm/gtc/type_ptr.hpp>



namespace black_label {
namespace rendering {

struct material 
{
	friend void swap( material& lhs, material& rhs )
	{
		using std::swap;
		swap(lhs.enabled, rhs.enabled);
		swap(lhs.ambient, rhs.ambient);
		swap(lhs.diffuse, rhs.diffuse);
		swap(lhs.specular, rhs.specular);
		swap(lhs.emissive, rhs.emissive);
		swap(lhs.alpha, rhs.alpha);
		swap(lhs.shininess, rhs.shininess);
		swap(lhs.ambient_texture, rhs.ambient_texture);
		swap(lhs.diffuse_texture, rhs.diffuse_texture);
		swap(lhs.specular_texture, rhs.specular_texture);
		swap(lhs.height_texture, rhs.height_texture);
	}

	material() : enabled{false} {}
	material( const material& other ) = default;
	material( material&& other ) : material{} { swap(*this, other); }
	material& operator=( material rhs ) { swap(*this, rhs); return *this; }
	// Implicitly constructible
	material( argument::ambient ambient ) : enabled{true}, ambient{ambient} {}
	material( argument::diffuse diffuse ) : enabled{true}, diffuse{diffuse} {}
	material( argument::specular specular ) : enabled{true}, specular{specular} {}
	material( argument::emissive emissive ) : enabled{true}, emissive{emissive} {}
	material( argument::alpha alpha ) : enabled{true}, alpha{alpha} {}
	material( argument::shininess shininess ) : enabled{true}, shininess{shininess} {}

	material& set( argument::ambient ambient ) 
	{ this->ambient = ambient; return *this; }
	material& set( argument::diffuse diffuse ) 
	{ this->diffuse = diffuse; return *this; }
	material& set( argument::specular specular ) 
	{ this->specular = specular; return *this; }
	material& set( argument::emissive emissive ) 
	{ this->emissive = emissive; return *this; }
	material& set( argument::alpha alpha ) 
	{ this->alpha = alpha; return *this; }
	material& set( argument::shininess shininess ) 
	{ this->shininess = shininess; return *this; }


	

	explicit operator bool() const { return enabled; }

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
	boost::filesystem::path ambient_texture, diffuse_texture, specular_texture, 
		height_texture;
};

inline material operator+( material material, argument::ambient ambient )
{ return std::move(material.set(ambient)); }
inline material operator+( material material, argument::diffuse diffuse )
{ return std::move(material.set(diffuse)); }
inline material operator+( material material, argument::specular specular )
{ return std::move(material.set(specular)); }
inline material operator+( material material, argument::emissive emissive )
{ return std::move(material.set(emissive)); }
inline material operator+( material material, argument::alpha alpha )
{ return std::move(material.set(alpha)); }
inline material operator+( material material, argument::shininess shininess )
{ return std::move(material.set(shininess)); }

} // namespace rendering
} // namespace black_label



#endif