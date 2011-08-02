#ifdef	WIN32
#define BLACK_LABEL_SHARED_LIBRARY extern "C" __declspec(dllexport)
#else
#define BLACK_LABEL_SHARED_LIBRARY extern "C"
#endif
