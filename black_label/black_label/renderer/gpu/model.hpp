#ifndef BLACK_LABEL_RENDERER_GPU_MODEL_HPP
#define BLACK_LABEL_RENDERER_GPU_MODEL_HPP

#include <black_label/renderer/gpu/mesh.hpp>
#include <black_label/renderer/light.hpp>

#include <boost/serialization/access.hpp>



namespace black_label {
namespace renderer {
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
	model( const cpu::model& cpu_model, texture_map& textures ) 
		: lights(cpu_model.lights)
		, checksum{cpu_model.checksum}
	{
		for (const auto& cpu_mesh : cpu_model.meshes) 
			meshes.emplace_back(gpu::mesh::configuration{cpu_mesh}, textures);
	}
	model( const model& other ) = delete; // Possible, but do you really want to?
	model( model&& other ) : model{} { swap(*this, other); }

	model& operator=( model rhs ) { swap(*this, rhs); return *this; }

	bool is_loaded() const { return !meshes.empty(); }
	bool has_lights() const { return !lights.empty(); }

	void render( const core_program& program, unsigned int texture_unit ) const
	{ for (const auto& mesh : meshes) mesh.render(program, texture_unit); }
	void render_without_material() const
	{ for (const auto& mesh : meshes) mesh.render_without_material(); }

	void clear() { meshes.clear(); lights.clear(); }



	mesh_container meshes;
	light_container lights;
	utility::checksum checksum;
};

} // namespace gpu
} // namespace renderer
} // namespace black_label



#endif
