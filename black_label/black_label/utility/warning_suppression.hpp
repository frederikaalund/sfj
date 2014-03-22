#ifndef BLACK_LABEL_UTILITY_WARNING_SUPPRESSION_HPP
#define BLACK_LABEL_UTILITY_WARNING_SUPPRESSION_HPP



namespace black_label {
namespace utility {

#ifdef MSVC
  #define MSVC_PUSH_WARNINGS(warnings) \
  __pragma(warning(push)) \
  __pragma(warning(disable : warnings))

  #define MSVC_POP_WARNINGS() \
  __pragma(warning(pop))
#else
  #define MSVC_PUSH_WARNINGS(warnings)
  #define MSVC_POP_WARNINGS()
#endif

} // namespace utility
} // namespace black_label



#endif
