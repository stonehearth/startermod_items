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
uint32 default_log_level_;
static uint32 console_log_severity_;

static const uint32 DEFAULT_LOG_LEVEL = 3;
static const uint32 DEFAULT_CONSOLE_LOG_SEVERITY = 3;

boost::log::sources::severity_logger< int > __radiant_log_source;

BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", int)

static boost::shared_ptr< boost::log::sinks::synchronous_sink< boost::log::sinks::text_file_backend > > file_log_sink;

void radiant::logger::Init(boost::filesystem::path const& logfile)
{
   file_log_sink = boost::log::add_file_log(
      keywords::file_name = logfile.string(),
      keywords::auto_flush = true,
      keywords::format = "%TimeStamp%%Message%"
   );
   boost::log::add_console_log(
      std::cout,
      keywords::filter = severity <= console_log_severity_
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

// Returns the configure log level for a particular key, or the default
// value if that key is not specified.
static inline uint32 GetLogLevel(std::string const& key, uint32 deflt)
{
   core::Config& config = core::Config::GetInstance();
   
   static const int sentinel = -1337;
   int log_level = config.Get<int>(key, sentinel);
   if (log_level != sentinel) {
      return log_level;
   }
   return config.Get<int>(key + ".log_level", deflt);
}

static void InitDefaults()
{
   core::Config& config = core::Config::GetInstance();

   default_log_level_ = config.Get<int>("logging.log_level", DEFAULT_LOG_LEVEL);
   console_log_severity_ = config.Get<int>("logging.console_log_severity", DEFAULT_CONSOLE_LOG_SEVERITY);

   if (console_log_severity_ != DEFAULT_CONSOLE_LOG_SEVERITY) {
      LOG_(0) << "console log severity " << console_log_severity_;
   }
   if (default_log_level_ != DEFAULT_LOG_LEVEL) {
      LOG_(0) << "setting default log level to " << default_log_level_;
   }
}

static void SetLogLevels()
{
   core::Config& config = core::Config::GetInstance();

   // Set the log_levels_ variable based on values in the config file.  This structure is generated
   // at compile time based on the radiant_log_categories.h header, so this is a bit tricky.
   // The only way we have of mapping the names in the config file to the structure fields is the
   // RADIANT_LOG_CATEGORIES macro.  As we're "evaluating" the macro, maintain a stack of keys
   // for the current category and the default log level at that category.  BEGIN_GROUP pushes
   // items onto the stack and END_GROUP pops them off.

   std::vector<std::string> config_path;
   std::vector<int> log_levels;
   config_path.push_back("logging");
   log_levels.push_back(default_log_level_);

   uint32* offset = reinterpret_cast<uint32*>(&log_levels_);
   uint32 level;

   // Push the current default log level and group name onto the stack
#define BEGIN_GROUP(group)        config_path.push_back(config_path.back() + "." #group); \
                                  log_levels.push_back(GetLogLevel(config_path.back(), log_levels.back()));

   // Generate the key in the config database by looking whatever's at the back of the config_path
   // stack and read the default.  Set the value by writing to the current offset in the log_levels_
   // structure.  This same macro was used to genearte the structure, so we know we're writing to the
   // correct space (in theory... assuming we always write/consume exactly the same amount of space
   // in the two expansions of RADIANT_LOG_CATEGORIES... structure padding might screw us here on odd
   // systems).
#define ADD_CATEGORY(category)    level = GetLogLevel(config_path.back() + "." #category, log_levels.back()); \
                                  if (level != default_log_level_) { \
                                     LOG_(0) << "setting " << config_path.back() << "." #category << " to " << level; \
                                  } \
                                  *offset++ = level; \

   // Easy.  Just pop current log_level and config path off the stack.
#define END_GROUP(group)          config_path.pop_back(); \
                                  log_levels.pop_back();

         RADIANT_LOG_CATEGORIES

#undef BEGIN_GROUP
#undef END_GROUP
#undef BEGIN_GROUP
}

void radiant::logger::InitLogLevels()
{
   InitDefaults();
   SetLogLevels();
}

void radiant::logger::Exit()
{
   LOG_(0) << "logger shutting down";
   platform_exit();
}
