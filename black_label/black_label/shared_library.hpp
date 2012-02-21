// Version 0.1
#ifndef BLACK_LABEL_SHARED_LIBRARY_SHARED_LIBRARY_HPP
#define BLACK_LABEL_SHARED_LIBRARY_SHARED_LIBRARY_HPP
#include <iostream>
#include "black_label/shared_library/utility.hpp"



namespace black_label
{
namespace shared_library
{

class shared_library
{
public:
	typedef void* handle_type;
	typedef void log_type;
	typedef void write_to_log_type( 
		log_type* log,
		int verbosity_level,
        char const* message,
		... );

	shared_library( 
		char const* path, 
		log_type* log, 
		write_to_log_type* write_to_log );
	~shared_library();

	bool is_open();
	int map_symbols( 
		size_t count,
		char const* names[],
		void** pointers[] );

	char const* path;
	handle_type handle;

	log_type* log;
	write_to_log_type* write_to_log;
protected:
private:
};

} // namespace shared_library
} // namespace black_label



#endif
