#ifndef BLACK_LABEL_RENDERING_GPU_ARGUMENT_MESH_HPP
#define BLACK_LABEL_RENDERING_GPU_ARGUMENT_MESH_HPP

#include <black_label/utility/range.hpp>



namespace black_label {
namespace rendering {
namespace gpu {
namespace argument {

class vertices : public utility::pointer_range<const float>
{ using utility::pointer_range<const float>::pointer_range; };
class normals : public utility::pointer_range<const float>
{ using utility::pointer_range<const float>::pointer_range; };
class texture_coordinates : public utility::pointer_range<const float>
{ using utility::pointer_range<const float>::pointer_range; };
class indices : public utility::pointer_range<const unsigned int>
{ using utility::pointer_range<const unsigned int>::pointer_range; };

} // namespace argument
} // namespace gpu
} // namespace rendering
} // namespace black_label



#endif
