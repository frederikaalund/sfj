#ifndef BLACK_LABEL_RENDERING_TYPES_AND_CONSTANTS_HPP
#define BLACK_LABEL_RENDERING_TYPES_AND_CONSTANTS_HPP

#include <black_label/path.hpp>

#include <memory>

#include <boost/serialization/access.hpp>

#include <tbb/concurrent_hash_map.h>



namespace black_label {
namespace rendering {

class rendering;



////////////////////////////////////////////////////////////////////////////////
/// Types and Constants
////////////////////////////////////////////////////////////////////////////////
namespace gpu {
namespace target {
	using type = unsigned int;
} // namespace target
} // namespace gpu

struct generate_type { generate_type() {} };
const generate_type generate;



////////////////////////////////////////////////////////////////////////////////
/// Render Mode Type
////////////////////////////////////////////////////////////////////////////////
class draw_mode
{
public:
	friend class boost::serialization::access;

	using type = unsigned int;

	draw_mode();
	draw_mode( type internal )
		: internal{internal} {}
	operator type() const { return internal; }

	template<typename archive_type>
	void serialize( archive_type& archive, unsigned int version )
	{ archive & internal; }

	type internal;
};



////////////////////////////////////////////////////////////////////////////////
/// Containers
////////////////////////////////////////////////////////////////////////////////
template<typename resource>
using resource_map = tbb::concurrent_hash_map<path, std::weak_ptr<resource>>;



} // namespace rendering
} // namespace black_label



#endif
