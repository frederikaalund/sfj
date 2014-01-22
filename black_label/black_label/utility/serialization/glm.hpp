#ifndef BLACK_LABEL_UTILITY_SERIALIZATION_GLM_HPP
#define BLACK_LABEL_UTILITY_SERIALIZATION_GLM_HPP

#include <glm/glm.hpp>



namespace boost {
namespace serialization {
	
template<typename archive_type, typename T, enum glm::precision P>
void serialize( archive_type& archive, glm::detail::tvec3<T, P>& vec, unsigned int version )
{
	archive & vec[0] & vec[1] & vec[2];
}

template<typename archive_type, typename T, enum glm::precision P>
void serialize( archive_type& archive, glm::detail::tvec4<T, P>& vec, unsigned int version )
{
	archive & vec[0] & vec[1] & vec[2] & vec[3];
}

} // namespace serialization
} // namespace boost



#endif
