// Version 0.1
#ifndef BLACK_LABEL_FILE_SYSTEM_WATCHER_TYPES_AND_CONSTANTS_HPP
#define BLACK_LABEL_FILE_SYSTEM_WATCHER_TYPES_AND_CONSTANTS_HPP

#include <black_label/utility/threading_building_blocks/path.hpp>
#include <black_label/utility/warning_suppression.hpp>

#include <cstdint>



namespace black_label {
namespace file_system_watcher {

using filters_type = std::int_least32_t;

enum class filter
{
	write = 1 << 0,
	access = 1 << 1,
	file_name = 1 << 2
};

constexpr filter operator|( filter lhs, filter rhs )
{
	using underlying_type = std::underlying_type<filter>::type;
	return static_cast<filter>(
		static_cast<underlying_type>(lhs) | static_cast<underlying_type>(rhs));
}

MSVC_PUSH_WARNINGS(4800)
constexpr bool operator&( filter lhs, filter rhs )
{
	using underlying_type = std::underlying_type<filter>::type;
	return static_cast<bool>(
		static_cast<underlying_type>(lhs) & static_cast<underlying_type>(rhs));
}
MSVC_POP_WARNINGS()

} // namespace file_system_watcher
} // namespace black_label



#endif
