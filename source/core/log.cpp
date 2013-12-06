#include "radiant.h"
#include "log_internal.h"
#include "core/config.h"
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
namespace attrs = boost::log::attributes;

radiant::logger::LogCategories log_levels_;

static const uint32 DEFAULT_LOG_LEVEL = 3;
static const uint32 DEFAULT_CONSOLE_LOG_SEVERITY = 3;

boost::log::sources::severity_logger< int > __radiant_log_source;

BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", int)

static boost::shared_ptr< boost::log::sinks::synchronous_sink< boost::log::sinks::text_file_backend > > file_log_sink;

void radiant::logger::Init(boost::filesystem::path const& logfile)
{
   file_log_sink = boost::log::add_file_log(
      keywords::file_name = logfile.string(),
      keywords::auto_flush = false,
      keywords::format = "%TimeStamp%%Message%"
   );
   boost::log::add_console_log(
      std::cout,
      keywords::filter = severity <= log_levels_.console_log_severity
   );
   boost::log::add_common_attributes();

   platform_init();
   LOG_(0) << "logger initialized";
}

void radiant::logger::Flush()
{
   if (file_log_sink) {
      file_log_sink->flush();
   }
}

void radiant::logger::InitLogLevels()
{
   core::Config& config = core::Config::GetInstance();

   uint32 default_log_level = config.Get<int>("logging.console_log_severity", DEFAULT_LOG_LEVEL);
   log_levels_.console_log_severity = config.Get<int>("logging.console_log_severity", DEFAULT_CONSOLE_LOG_SEVERITY);

   uint32 *first = reinterpret_cast<uint32*>(&log_levels_);
   uint32 *last = reinterpret_cast<uint32*>((&log_levels_) + 1);
   while (first != last) {
      *first++ = default_log_level;
   }
#if 0

#define BEGIN_GROUP(group)        struct {
#define ADD_CATEGORY(category)         uint32 category;
#define END_GROUP(group)          } group;

         RADIANT_LOG_CATEGORIES

#undef BEGIN_GROUP
#undef END_GROUP
#undef BEGIN_GROUP
#endif
}

void radiant::logger::Exit()
{
   LOG_(0) << "logger shutting down";
   platform_exit();
}
