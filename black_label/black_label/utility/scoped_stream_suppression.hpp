#ifndef BLACK_LABEL_UTILITY_SCOPED_STREAM_SUPPRESSION_HPP
#define BLACK_LABEL_UTILITY_SCOPED_STREAM_SUPPRESSION_HPP



#ifdef MSVC_VERSION // TODO: Can this include be removed?
#include <io.h>
#endif
#include <fcntl.h>



namespace black_label {
namespace utility {

class scoped_stream_suppression
{
public:
	scoped_stream_suppression( FILE* stream ) : stream(stream)
	{
#if defined UNIX
		fflush(stream);
		original = dup(fileno(stream));
		auto nulled = open("/dev/null", O_WRONLY);
		dup2(nulled, fileno(stream));
		close(nulled);
#elif defined WINDOWS
		fflush(stream);
		original = _dup(_fileno(stream));
		auto nulled = _open("nul", _O_WRONLY);
		_dup2(nulled, _fileno(stream));
		_close(nulled);
#endif
	}

	~scoped_stream_suppression()
	{
#if defined UNIX
		fflush(stream);
		dup2(original, fileno(stream));
		close(original);
#elif defined WINDOWS
		fflush(stream);
		_dup2(original, _fileno(stream));
		_close(original);
#endif
	}

	FILE* stream;
	int original;
};

} // namespace utility
} // namespace black_label



#endif
