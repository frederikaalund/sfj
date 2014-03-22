#ifndef BLACK_LABEL_RENDERER_CPU_MODEL_HPP
#define BLACK_LABEL_RENDERER_CPU_MODEL_HPP

#include <black_label/file_buffer.hpp>
#include <black_label/renderer/cpu/mesh.hpp>
#include <black_label/renderer/light.hpp>
#include <black_label/utility/cache_file.hpp>

#include <vector>



namespace black_label {
namespace renderer {
namespace cpu {

typedef boost::crc_32_type::value_type checksum_type;

class model : public utility::cache_file
{
public:
	using light_container = std::vector<light>;
	using mesh_container = std::vector<mesh>;

	friend class boost::serialization::access;



	friend void swap( model& lhs, model& rhs )
	{
		using std::swap;
		swap(lhs.meshes, rhs.meshes);
		swap(lhs.lights, rhs.lights);
		swap(static_cast<cache_file&>(lhs), static_cast<cache_file&>(rhs));
	}



	model() {}
	model( path path, defer_import_type ) : cache_file{path} {}
	explicit model( path path ) : model{path, defer_import}
	{ import(std::move(path)); }
	model( const model& ) = delete;
	model( model&& other ) { swap(*this, other); }
	model& operator=( model rhs ) { swap(*this, rhs); return *this; }
	
	bool import( path path );
#ifdef DEVELOPER_TOOLS
#ifndef NO_FBX
	bool import_fbxsdk( path path );
#endif
	bool import_assimp( path path );
#endif // #ifdef DEVELOPER_TOOLS

	bool is_empty() const { return meshes.empty(); }
	explicit operator bool() const { return !is_empty(); }



	template<typename archive_type>
	void serialize( archive_type& archive, unsigned int version )
	{ archive & checksum & meshes & lights; }



	mesh_container meshes;
	light_container lights;



private:
#ifdef DEVELOPER_TOOLS
#ifndef NO_FBX
#endif
#endif
};

} // namespace cpu
} // namespace renderer
} // namespace black_label



#endif
