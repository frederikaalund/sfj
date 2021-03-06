#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/file_buffer.hpp>

#include <fstream>

using namespace std;



namespace black_label {
namespace file_buffer {

void file_buffer::load_file( const char* path_to_file )
{
	ifstream shader_source(path_to_file, ios::ate | ios::binary);
	if (!shader_source.is_open()) return;

	buffer = buffer_type(static_cast<buffer_type::size_type>(
		shader_source.tellg()));
	shader_source.seekg(0, ios::beg);
	shader_source.read(buffer.data(), buffer.size());
}

void file_buffer::load_file( const char* path_to_file, null_terminated_type )
{
	ifstream shader_source(path_to_file, ios::ate | ios::binary);
	if (!shader_source.is_open()) return;

	buffer = buffer_type(static_cast<buffer_type::size_type>(
		shader_source.tellg())+1);
	shader_source.seekg(0, ios::beg);
	shader_source.read(buffer.data(), buffer.size()-1);
	buffer[buffer.size()-1] = '\0';
}

} // namespace file_buffer
} // namespace black_label
