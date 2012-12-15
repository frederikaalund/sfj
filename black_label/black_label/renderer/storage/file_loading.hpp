#ifndef BLACK_LABEL_RENDERER_STORAGE_FILE_LOADING_HPP
#define BLACK_LABEL_RENDERER_STORAGE_FILE_LOADING_HPP

#ifdef DEVELOPER_TOOLS

#include <black_label/shared_library/utility.hpp>
#include <black_label/renderer/storage/cpu/model.hpp>
#include <black_label/renderer/storage/gpu/model.hpp>

#define BOOST_FILESYSTEM_NO_DEPRECATED
#include <boost/filesystem/convenience.hpp>



namespace black_label {
namespace renderer {
namespace storage {

BLACK_LABEL_SHARED_LIBRARY bool load_fbx( 
	const boost::filesystem::path& path, 
	cpu::model& cpu_model,
	gpu::model& gpu_model);

BLACK_LABEL_SHARED_LIBRARY bool load_assimp( 
	const boost::filesystem::path& path, 
	cpu::model& cpu_model, 
	gpu::model& gpu_model );

////////////////////////////////////////////////////////////////////////////////
/// Unfortunately, Assimp reads files from memory differently than from a file
/// path.
/// 
/// Maybe the following can be used in the future:
/// http://assimp.sourceforge.net/lib_html/class_assimp_1_1_i_o_system.html
////////////////////////////////////////////////////////////////////////////////
// BLACK_LABEL_SHARED_LIBRARY bool load_assimp( 
// 	const void* data,
// 	size_t size, 
// 	cpu::model& cpu_model, 
// 	gpu::model& gpu_model );



} // namespace storage
} // namespace renderer
} // namespace black_label



#endif

#endif
