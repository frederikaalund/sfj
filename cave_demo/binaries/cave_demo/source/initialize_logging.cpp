#include <cave_demo/initialize_logging.hpp>

#include <boost/log/attributes.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/utility/empty_deleter.hpp>



namespace cave_demo {



void initialize_logging() {
	using namespace boost::log;
	using boost::make_shared;
	using boost::empty_deleter;

	auto core = boost::log::core::get();



	// File
	auto file_backend = make_shared<sinks::text_file_backend>(
		keywords::file_name = "./logs/log_%N.txt",
		keywords::rotation_size = 5 * 1024 * 1024);
	//file_backend->set_file_collector(sinks::file::make_collector(
	//	keywords::target = "./logs/",
	//	keywords::max_size = 10 * 1024 * 1024, // 10 MiB
	//	keywords::min_free_space = 50 * 1024 * 1024)); // 50 MiB	
	//file_backend->scan_for_files();

	auto file_sink = make_shared<sinks::synchronous_sink<sinks::text_file_backend> >(
		file_backend);

	file_sink->set_formatter(
		expressions::stream
		<< "[" << expressions::attr<trivial::severity_level>("Severity")
		<< "] " << expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%d.%m.%Y %H:%M:%S.%f")
		<< " (" << expressions::format_date_time<boost::posix_time::time_duration>("Uptime", "%H:%M:%S.%f") << "):"
		<< expressions::if_(expressions::has_attr<bool>("MultiLine"))
		[expressions::stream << "\n"]
	.else_
		[expressions::stream << " "]
	<< expressions::message);


	// Console
	auto clog_backend = make_shared<sinks::text_ostream_backend>();
	clog_backend->add_stream(boost::shared_ptr<std::ostream>(&std::clog, boost::empty_deleter()));
	clog_backend->auto_flush(true);

	auto clog_sink = make_shared<sinks::synchronous_sink<sinks::text_ostream_backend> >(
		clog_backend);

	clog_sink->set_formatter(
		expressions::stream
		<< "[" << expressions::attr<trivial::severity_level>("Severity")
		<< "] " << expressions::format_date_time<boost::posix_time::time_duration>("Uptime", "%M:%S.%f") << ":"
		<< expressions::if_(expressions::has_attr<bool>("MultiLine"))
		[expressions::stream << "\n"]
	.else_
		[expressions::stream << " "]
	<< expressions::message);

	core->add_global_attribute("TimeStamp", attributes::local_clock());
	core->add_global_attribute("Uptime", attributes::timer());
	core->add_sink(file_sink);
	core->add_sink(clog_sink);
}



} // namespace cave_demo
