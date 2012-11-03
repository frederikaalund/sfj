#ifndef BLACK_LABEL_RENDERER_MATERIAL_HPP
#define BLACK_LABEL_RENDERER_MATERIAL_HPP

#include <black_label/shared_library/utility.hpp>

#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>



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


template<typename stream_char_type, typename string_char_type>
std::basic_istream<stream_char_type>& read_binary( 
	std::basic_istream<stream_char_type>& stream, 
	std::basic_string<string_char_type>& string )
{
	typedef std::basic_string<string_char_type> string_type;

	string_type::size_type size;
	stream.read(reinterpret_cast<stream_char_type*>(&size), sizeof(string_type::size_type));
	if (0 < size)
	{
		vector<std::string::value_type> buffer(size);
		stream.read(reinterpret_cast<stream_char_type*>(buffer.data()), sizeof(string_type::value_type) * size);
		string.assign(buffer.data(), size);
	}
	return stream;
}

template<typename stream_char_type, typename string_char_type>
std::basic_ostream<stream_char_type>& write_binary( 
	std::basic_ostream<stream_char_type>& stream, 
	const std::basic_string<string_char_type>& string )
{
	typedef std::basic_string<string_char_type> string_type;

	auto size = string.size();
	stream.write(reinterpret_cast<const stream_char_type*>(&size), sizeof(string_type::size_type));
	stream.write(reinterpret_cast<const stream_char_type*>(string.data()), sizeof(string_type::value_type) * size);
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
		read_binary(stream, material.diffuse_texture);
		return stream;
	}

	template<typename char_type>
	friend std::basic_ostream<char_type>& operator<<( 
		std::basic_ostream<char_type>& stream, 
		const material& material )
	{
		stream << material.diffuse;
		stream.write(reinterpret_cast<const char_type*>(&material.alpha), sizeof(float));
		write_binary(stream, material.diffuse_texture);
		return stream;
	}

	glm::vec3 diffuse;
	float alpha;
	std::string diffuse_texture;
};

} // namespace renderer
} // namespace black_label



#endif