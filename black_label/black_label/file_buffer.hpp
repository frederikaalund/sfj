// Version 0.1
#ifndef BLACK_LABEL_DARRAY_HPP
#define BLACK_LABEL_DARRAY_HPP

#include <black_label/shared_library/utility.hpp>
#include <black_label/utility/warning_suppression.hpp>

#include <string>
#include <vector>



namespace black_label {
namespace file_buffer {

////////////////////////////////////////////////////////////////////////////////
/// Types and Constants
////////////////////////////////////////////////////////////////////////////////
struct null_terminated_type { null_terminated_type() {} };
const null_terminated_type null_terminated;



////////////////////////////////////////////////////////////////////////////////
/// File Buffer
////////////////////////////////////////////////////////////////////////////////
class BLACK_LABEL_SHARED_LIBRARY file_buffer
{
public:
	typedef std::vector<char> buffer_type;
	
	file_buffer( const std::string& path_to_file )
	{ load_file(path_to_file.c_str()); }
	file_buffer( const std::string& path_to_file, null_terminated_type nt )
	{ load_file(path_to_file.c_str(), nt); }
	file_buffer( const char* path_to_file )
	{ load_file(path_to_file); }
	file_buffer( const char* path_to_file, null_terminated_type nt )
	{ load_file(path_to_file, nt); }

	buffer_type::size_type size() const 
	{ return buffer.size(); }
	const buffer_type::value_type* data() const
	{ return buffer.data(); }
	bool empty() const { return nullptr == data(); }

MSVC_PUSH_WARNINGS(4251)
	buffer_type buffer;
MSVC_POP_WARNINGS()

  

protected:
	void load_file( const char* path_to_file );
	void load_file( const char* path_to_file, null_terminated_type );
};

} // namespace file_buffer
} // namespace black_label



#endif
