#ifndef _RADIANT_LOGGER_H
#define _RADIANT_LOGGER_H

#include <boost/log/core.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/filesystem.hpp>
#include "radiant_log_categories.h"

namespace radiant {
   namespace logger {
      void Init(boost::filesystem::path const& logfile);
      void InitLogLevels();
      void Flush();
      void Exit();

      struct LogCategories {
         // Log lines <= console_log_severity will be written to the debug console as well as
         // the logfile
         uint32   console_log_severity;

         // Create entries for all the log levels...
#define BEGIN_GROUP(group)        struct {
#define ADD_CATEGORY(category)         uint32 category;
#define END_GROUP(group)          } group;

         RADIANT_LOG_CATEGORIES

#undef BEGIN_GROUP
#undef END_GROUP
#undef BEGIN_GROUP
      };
   };
};

// _LOG writes to the logfile unconditionally.  This is not the macro
// you're looking for.
#define LOG_(x)  BOOST_LOG_SEV(__radiant_log_source, x)

// Used to write unconditionally to the log for critical messages.  Use
// extremely sparingly.
#define LOG_CRITICAL() LOG_(0)

// LOG_CATEGORY writes to the log using a very specialized category string.
// Useful when you want the log to contain context which might change each
// time you execute the log line (e.g. the id of the current object).
#define LOG_CATEGORY(category, level, prefix) if (log_levels_.category >= level) LOG_(level) << " | " << level << " | " << prefix << " | "

// LOG writes to the log if the log level at the specified category is
// greater or equal to the level passed in the macro.  Yes, this means
// LOG(xxx, 0) is logged unconditionally. Don't abuse it!
#define LOG(category, level)  LOG_CATEGORY(category, level, #category)

// Extern variables used by the macros.  Do not reference these directly!
extern struct radiant::logger::LogCategories log_levels_;
extern boost::log::sources::severity_logger< int > __radiant_log_source;

#endif
