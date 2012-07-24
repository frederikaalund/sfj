#ifdef WINDOWS
	#ifdef BLACK_LABEL_SHARED_LIBRARY_EXPORT
		#define BLACK_LABEL_SHARED_LIBRARY __declspec(dllexport)
	#else
		#define BLACK_LABEL_SHARED_LIBRARY __declspec(dllimport)
	#endif
#else
	#define BLACK_LABEL_SHARED_LIBRARY 
#endif
