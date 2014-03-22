#ifndef BLACK_LABEL_RENDERER_ARGUMENT_MATERIAL_HPP
#define BLACK_LABEL_RENDERER_ARGUMENT_MATERIAL_HPP

#include <glm/glm.hpp>



namespace black_label {
namespace renderer {
namespace argument {



namespace detail {

template<typename T>
class single {
public:
	using value_type = T;
	single() {}
	single( T value ) : value{value} {}
	operator T() const { return value; }
	T value;
};

} // namespace detail



class ambient : public glm::vec3 { public: using glm::vec3::vec3; };
class diffuse : public glm::vec3 { public: using glm::vec3::vec3; };
class specular : public glm::vec3 { public: using glm::vec3::vec3; };
class emissive : public glm::vec3 { public: using glm::vec3::vec3; };
class alpha : public black_label::renderer::argument::detail::single<float> { public: using detail::single<float>::single; };
class shininess : public black_label::renderer::argument::detail::single<float> { public: using detail::single<float>::single; };



} // namespace argument
} // namespace renderer
} // namespace black_label



#endif