#include "radiant.h"
#include "log_internal.h"
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

using namespace ::radiant;
using namespace ::radiant::logger;

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

boost::log::sources::severity_logger< Severity > __radiant_log_source;

BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", ::radiant::logger::Severity)

void radiant::logger::init()
{
   boost::log::add_file_log(
      keywords::file_name = "stonehearth.log",
      keywords::auto_flush = true
   );
   boost::log::add_console_log(
      std::cout,
      keywords::filter = severity >= ::radiant::logger::WARNING
   );
   boost::log::add_common_attributes();

   platform_init();
}

void radiant::logger::exit()
{
   platform_exit();
}
