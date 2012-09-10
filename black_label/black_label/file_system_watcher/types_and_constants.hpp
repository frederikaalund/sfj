// Version 0.1
#ifndef BLACK_LABEL_FILE_SYSTEM_WATCHER_TYPES_AND_CONSTANTS_HPP
#define BLACK_LABEL_FILE_SYSTEM_WATCHER_TYPES_AND_CONSTANTS_HPP

#include <cstdint>
#include <string>

namespace black_label
{
namespace file_system_watcher
{

typedef std::string path_type;

typedef std::int_least32_t filters_type;
const filters_type FILTER_WRITE = 0;

} // namespace file_system_watcher
} // namespace black_label



#endif
