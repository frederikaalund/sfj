// Version 0.1
#ifndef BLACK_LABEL_LOG_LOG_HPP
#define BLACK_LABEL_LOG_LOG_HPP

#include <fstream>
#include <stdarg.h>



namespace black_label
{
namespace log
{

class log
{
public:
	static void write_to_log( 
		log* log, 
		int verbosity_level, 
		char const* message, 
		... );

	log( char const* log_file, bool write_log_events = true );
	~log();

	bool is_open();
	void write( int verbosity_level, char const* message, ... );

	std::ofstream file;
	bool write_log_events;
protected:
private:
	void write_explicit( 
		int verbosity_level, 
        char const* message,
		va_list extra_arguments );
};

} // namespace log
} // namespace black_label



#endif