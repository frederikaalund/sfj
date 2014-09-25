#ifndef BLACK_LABEL_RENDERING_GPU_MODEL_HPP
#define BLACK_LABEL_RENDERING_GPU_MODEL_HPP

#include <black_label/rendering/gpu/mesh.hpp>
#include <black_label/rendering/light.hpp>

#include <boost/serialization/access.hpp>



namespace black_label {
namespace rendering {
namespace gpu {

class model
{
public:
	using mesh_container = std::vector<mesh>;
	using light_container = cpu::model::light_container;



	friend void swap( model& lhs, model& rhs )
	{
		using std::swap;
		swap(lhs.meshes, rhs.meshes);
		swap(lhs.lights, rhs.lights);
		swap(lhs.checksum, rhs.checksum);
	}

	model() {}
	template<typename T>
	model( T&& cpu_model, texture_map& textures ) 
		: lights(std::forward<T>(cpu_model).lights)
		, checksum{std::forward<T>(cpu_model).checksum}
	{
		bool testing = std::is_rvalue_reference<T&&>::value;

		for (auto& cpu_mesh : std::forward<T>(cpu_model).meshes) 
			meshes.emplace_back(utility::forward_as<T>(cpu_mesh), textures);
	}
	model( model&& other ) : model{} { swap(*this, other); }

	model& operator=( model rhs ) { swap(*this, rhs); return *this; }

	bool is_loaded() const { return !meshes.empty(); }
	bool has_lights() const { return !lights.empty(); }

	void render( const core_program& program, unsigned int texture_unit ) const
	{ for (const auto& mesh : meshes) mesh.render(program, texture_unit); }
	void render() const
	{ for (const auto& mesh : meshes) mesh.render(); }



	mesh_container meshes;
	light_container lights;
	utility::checksum checksum;
};

} // namespace gpu
} // namespace rendering
} // namespace black_label



#endif
