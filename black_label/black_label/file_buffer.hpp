// Version 0.1
#ifndef BLACK_LABEL_DARRAY_HPP
#define BLACK_LABEL_DARRAY_HPP

#include <black_label/container/darray.hpp>



namespace black_label
{
namespace file_buffer
{

////////////////////////////////////////////////////////////////////////////////
/// Types and Constants
////////////////////////////////////////////////////////////////////////////////
struct null_terminated_type {};
const null_terminated_type null_terminated;



////////////////////////////////////////////////////////////////////////////////
/// File Buffer
////////////////////////////////////////////////////////////////////////////////
class file_buffer
{
public:
	typedef container::darray<char> buffer_type;

	
	file_buffer( const char* path_to_file );
	file_buffer( const char* path_to_file, null_terminated_type );

	buffer_type::size_type size() const 
	{ return buffer.capacity(); }
	const buffer_type::value_type* data() const
	{ return buffer.data(); }

	buffer_type buffer;
};

} // namespace file_buffer
} // namespace black_label



#endif
