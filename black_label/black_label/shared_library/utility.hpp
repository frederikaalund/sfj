#if defined WINDOWS && defined BLACK_LABEL_LINK_AS_SHARED_LIBRARIES
	#ifdef BLACK_LABEL_SHARED_LIBRARY_EXPORT
		#define BLACK_LABEL_SHARED_LIBRARY __declspec(dllexport)
	#else
		#define BLACK_LABEL_SHARED_LIBRARY __declspec(dllimport)
	#endif
#else
	#define BLACK_LABEL_SHARED_LIBRARY 
#endif



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
