#ifndef BLACK_LABEL_RENDERER_GPU_ARGUMENT_MESH_HPP
#define BLACK_LABEL_RENDERER_GPU_ARGUMENT_MESH_HPP

#include <black_label/renderer/cpu/texture.hpp>
#include <black_label/utility/range.hpp>



namespace black_label {
namespace renderer {
namespace gpu {
namespace argument {

class vertices : public utility::const_pointer_range<float>
{ using utility::const_pointer_range<float>::const_pointer_range; };
class normals : public utility::const_pointer_range<float>
{ using utility::const_pointer_range<float>::const_pointer_range; };
class texture_coordinates : public utility::const_pointer_range<float>
{ using utility::const_pointer_range<float>::const_pointer_range; };
class indices : public utility::const_pointer_range<unsigned int>
{ using utility::const_pointer_range<unsigned int>::const_pointer_range; };

} // namespace argument
} // namespace gpu
} // namespace renderer
} // namespace black_label



#endif
