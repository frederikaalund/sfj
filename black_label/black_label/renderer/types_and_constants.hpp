#ifndef BLACK_LABEL_RENDERER_TYPES_AND_CONSTANTS_HPP
#define BLACK_LABEL_RENDERER_TYPES_AND_CONSTANTS_HPP

#include <boost/serialization/access.hpp>



namespace black_label {
namespace renderer {

class renderer;



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
class render_mode
{
public:
	friend class boost::serialization::access;

	using type = unsigned int;

	render_mode();
	render_mode( type internal )
		: internal{internal} {}
	operator type() const { return internal; }

	template<typename archive_type>
	void serialize( archive_type& archive, unsigned int version )
	{ archive & internal; }

	type internal;
};



} // namespace renderer
} // namespace black_label



#endif
