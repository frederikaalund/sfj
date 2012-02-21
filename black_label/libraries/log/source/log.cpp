#include "black_label/log.hpp"

#include <cstdio>
#include <cstring>
#include <time.h>



namespace black_label
{
namespace log
{

using namespace std;



void log::write_to_log( log* log, int verbosity_level, char const* message, ... )
{
	va_list extra_arguments;
	va_start(extra_arguments, message);
	log->write_explicit(verbosity_level, message, extra_arguments);
	va_end(extra_arguments);
}



log::log( char const* log_file, bool write_log_events )
	: write_log_events(write_log_events)
{
	file.open(log_file, ios::app);
	if (is_open())
	{
		file << endl;
		if (write_log_events) write(2, "Opened log.");
	}
}

log::~log()
{
	if (is_open() && write_log_events) write(2, "Closed log.");
	file.close();
}



bool log::is_open()
{
	return file.is_open();
}

void log::write( int verbosity_level, char const* message, ... )
{
	va_list extra_arguments;
	va_start(extra_arguments, message);
	write_explicit(verbosity_level, message, extra_arguments);
	va_end(extra_arguments);
}

void log::write_explicit( 
	int verbosity_level, 
	char const* message, 
	va_list extra_arguments )
{
	if (2 < verbosity_level) return;

	static char const* prefixes[] = {
		"[ERROR]   ",
		"[WARNING] ",
		"[INFO]    "};

	time_t raw_time;
	time(&raw_time);
	tm* local_time = localtime(&raw_time);
	char formatted_time[32];
	sprintf(formatted_time, "[%02i:%02i:%02i %02i-%02i-%i] ", 
		local_time->tm_hour, 
		local_time->tm_min,
		local_time->tm_sec,
		local_time->tm_mday,
		local_time->tm_mon+1,
		local_time->tm_year+1900);

	char formatted_message[1024];
	vsnprintf(formatted_message, 1024, message, extra_arguments);

	file << prefixes[verbosity_level] << formatted_time << formatted_message 
		<< endl;
}

} // namespace log
} // namespace black_label