#ifndef BLACK_LABEL_RENDERER_MATERIAL_HPP
#define BLACK_LABEL_RENDERER_MATERIAL_HPP

#include <black_label/shared_library/utility.hpp>

#include <glm/glm.hpp>



namespace black_label
{
namespace renderer
{

////////////////////////////////////////////////////////////////////////////////
/// Stream Operator Overloads
////////////////////////////////////////////////////////////////////////////////
template<typename char_type>
std::basic_istream<char_type>& operator>>( 
	std::basic_istream<char_type>& stream, 
	glm::vec3& v )
{
	stream.read(reinterpret_cast<char_type*>(glm::value_ptr(v)), sizeof(glm::vec3));
	return stream;
}

template<typename char_type>
std::basic_ostream<char_type>& operator<<( 
	std::basic_ostream<char_type>& stream, 
	const glm::vec3& v )
{
	stream.write(reinterpret_cast<const char_type*>(glm::value_ptr(v)), sizeof(glm::vec3));
	return stream;
}



////////////////////////////////////////////////////////////////////////////////
/// Material
////////////////////////////////////////////////////////////////////////////////
struct material 
{
	template<typename char_type>
	friend std::basic_istream<char_type>& operator>>( 
		std::basic_istream<char_type>& stream, 
		material& material )
	{
		stream >> material.diffuse;
		stream.read(reinterpret_cast<char_type*>(&material.alpha), sizeof(float));
		return stream;
	}

	template<typename char_type>
	friend std::basic_ostream<char_type>& operator<<( 
		std::basic_ostream<char_type>& stream, 
		const material& material )
	{
		stream << material.diffuse;
		stream.write(reinterpret_cast<const char_type*>(&material.alpha), sizeof(float));
		return stream;
	}

	glm::vec3 diffuse;
	float alpha;
};

} // namespace renderer
} // namespace black_label



#endif