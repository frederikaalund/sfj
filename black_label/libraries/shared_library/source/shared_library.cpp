#include <black_label/shared_library.hpp>

#ifdef WINDOWS
  #include <windows.h>
#elif UNIX
  #include <dlfcn.h>
#endif

#include <iostream>



namespace black_label
{
namespace shared_library
{

inline shared_library::handle_type load_shared_library( char const* path )
{
#ifdef WINDOWS
    return LoadLibrary(path);
#elif UNIX
    return dlopen(path, RTLD_LAZY);
#endif
}

inline void* get_symbol_address( shared_library::handle_type handle, char const* symbol_name )
{
#ifdef WINDOWS
	return GetProcAddress(reinterpret_cast<HMODULE>(handle), symbol_name);
#elif UNIX
    return dlsym(handle, symbol_name);
#endif
}



shared_library::shared_library( 
	char const* path,
	log_type* log,
	write_to_log_type* write_to_log )
	: path(path)
	, handle(load_shared_library(path))
	, log(log)
	, write_to_log(write_to_log)
{
	if (!handle) write_to_log(log, 0, "Library \"%s\" failed to open.", path);
}

shared_library::~shared_library()
{
    if (handle)
#ifdef WINDOWS
        FreeLibrary(reinterpret_cast<HMODULE>(handle));
#elif UNIX
        dlclose(handle);
#endif
}



bool shared_library::is_open()
{
	return 0 != handle;
}

int shared_library::map_symbols( size_t count, char const* names[], void** pointers[] )
{
	int unmapped_symbols = 0;
	for (size_t symbol = 0; symbol < count; ++symbol)
	{
		*pointers[symbol] = get_symbol_address(handle, names[symbol]);
		if (!*pointers[symbol])
		{
			write_to_log(log, 0, "Symbol \"%s\" not found.", names[symbol]);
			++unmapped_symbols;
		}
	}
	return unmapped_symbols;
}

} // namespace shared_library
} // namespace black_label
