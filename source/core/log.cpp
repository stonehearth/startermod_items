#include "radiant.h"
#include "core/config.h"
#include "platform/thread.h"
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

using namespace ::radiant;
using namespace ::radiant::log;

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace attrs = boost::log::attributes;

radiant::log::LogCategories log_levels_;
uint32 default_log_level_;
std::unordered_map<std::string, radiant::log::LogLevel> log_level_names_;
std::unordered_map<platform::ThreadId, std::string> log_thread_names_;

static uint32 console_log_severity_;
static const uint32 DEFAULT_LOG_LEVEL = 2;
static const uint32 DEFAULT_CONSOLE_LOG_SEVERITY = 2;

boost::log::sources::severity_logger< int > __radiant_log_source;

BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", int)

static boost::shared_ptr< boost::log::sinks::synchronous_sink< boost::log::sinks::text_file_backend > > file_log_sink;

void radiant::log::Init(boost::filesystem::path const& logfile)
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

   log_level_names_["error"] = ERROR;
   log_level_names_["warning"] = WARNING;
   log_level_names_["info"] = INFO;
   log_level_names_["debug"] = DEBUG;
   log_level_names_["detail"] = DETAIL;
   log_level_names_["spam"] = SPAM;

   LOG_(0) << "logger initialized";
}

int radiant::log::GetDefaultLogLevel()
{
   return default_log_level_;
}

void radiant::log::Flush()
{
   if (file_log_sink) {
      file_log_sink->flush();
   }
}

// Returns the configure log level for a particular key, or the default
// value if that key is not specified.
int log::GetLogLevel(std::string const& key, int deflt)
{
   core::Config& config = core::Config::GetInstance();
   static const int SENTINEL = 0xd3adb33f;
   
   auto getLogLevel = [](std::string const& name) -> int {
      JSONNode n = core::Config::GetInstance().Get<JSONNode>(name, JSONNode());
      if (n.type() == JSON_STRING) {
         auto i = log_level_names_.find(n.as_string());
         if (i != log_level_names_.end()) {
            return i->second;
         }
      } else if (n.type() == JSON_NUMBER) {
         return n.as_int();
      }
      return SENTINEL;
   };

   int level = getLogLevel(key);
   if (level != SENTINEL) {
      return level;
   }
   level = getLogLevel(BUILD_STRING(key << ".log_level"));
   if (level != SENTINEL) {
      return level;
   }
   return deflt;
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

void radiant::log::InitLogLevels()
{
   InitDefaults();
   SetLogLevels();
}

void radiant::log::Exit()
{
   LOG_(0) << "logger shutting down";
}

const char* radiant::log::GetCurrentThreadName()
{
   platform::ThreadId id = platform::GetCurrentThreadId();
   auto i = log_thread_names_.find(id);
   if (i != log_thread_names_.end()) {
      return i->second.c_str();
   }
   SetCurrentThreadName(BUILD_STRING("thread" << id));
   return GetCurrentThreadName();
}

void radiant::log::SetCurrentThreadName(std::string const& name)
{
   platform::ThreadId id = platform::GetCurrentThreadId();
   log_thread_names_[id] = name;
}
