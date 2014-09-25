#ifndef BLACK_LABEL_RENDERING_TYPES_AND_CONSTANTS_HPP
#define BLACK_LABEL_RENDERING_TYPES_AND_CONSTANTS_HPP

#include <boost/serialization/access.hpp>



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



} // namespace rendering
} // namespace black_label



#endif
