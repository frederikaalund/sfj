#define BLACK_LABEL_SHARED_LIBRARY_EXPORT
#include <black_label/renderer/cpu/texture.hpp>
#include <black_label/utility/scoped_stream_suppression.hpp>

#include <fstream>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/log/trivial.hpp>

#include <squish.h>

#include <SFML/Graphics/Image.hpp>

#include <GL/glew.h>



using namespace black_label::utility;
using namespace std;
using namespace boost::archive;
using namespace sf;



namespace black_label {
namespace renderer {
namespace cpu {

bool texture::import( path path )
{
	if (cache_file::import(path, *this))
		return true;
#ifdef DEVELOPER_TOOLS
	if (import_sfml(path))
	{
		compress();
		if (cache_file::export(path, *this))
			return true;
	}
#endif // #ifdef DEVELOPER_TOOLS

	BOOST_LOG_TRIVIAL(warning) << "Failed to import texture " << path;
	return false;
}



#ifdef DEVELOPER_TOOLS

bool texture::import_sfml( path path )
{
	BOOST_LOG_TRIVIAL(info) << "Importing texture using SFML " << path;

	Image image;
	{
		scoped_stream_suppression suppress(stdout);
		if (!image.loadFromFile(path.string()))
		 return false;
	}

	width = image.getSize().x;
	height = image.getSize().y;
	auto image_size = sizeof(sf::Uint8) * width * height * 4;
	auto image_begin = image.getPixelsPtr();
	auto image_end = &image_begin[image_size];
	data.assign(image_begin, image_end);

	
 
	BOOST_LOG_TRIVIAL(info) << "Imported texture " << path;
	return true;
}

void texture::compress()
{
	using namespace squish;
	auto flags = kDxt5 | kColourRangeFit;
	data_container compressed_data(GetStorageRequirements(width, height, flags));
	CompressImage(data.data(), width, height, compressed_data.data(), flags);
	data = std::move(compressed_data);
	compressed = true;
}

#endif // #ifdef DEVELOPER_TOOLS

} // namespace cpu
} // namespace renderer
} // namespace black_label