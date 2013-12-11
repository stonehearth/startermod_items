#ifndef _RADIANT_LOGGER_H
#define _RADIANT_LOGGER_H

#include <boost/log/core.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <iomanip>
#include "radiant_log_categories.h"

// Radiant Logging Module
// 
// Log levels are configured inside the "logging" key of the config object.  You can set "log_level"
// at any group level to change the log level for all categories in that group, or the key specifically
// to change just that level.  For example,
//
//     "logging" : {
//        "log_level" : 4,
//        "browser": 10,
//        "physics" : {
//           "log_level": 8
//        },
//        "renderer" : {
//           "log_level" : 3,
//           "mob": 1
//        }
//     }
// 
// 1) Sets the default log level for all categories to 4.  Categories not otherwise overriden will use
//    this value (e.g. core.guard).
// 2) Sets the log level of the browser group to 10
// 3) Sets the log level of all categories in the physics group to 8
// 4) Sets the log level of all categories in the renderer group to 3, except renderer.mob, which is 1.
//
// You can also set logging.console_log_severity to change the level at which things get put into
// the console window.  I don't recommend a high value, here, as spamming the console has a huge
// impact on game performance.
//

namespace radiant {
   namespace logger {
      void Init(boost::filesystem::path const& logfile);
      void InitLogLevels();
      void Flush();
      void Exit();

      struct LogCategories {
         // Log lines <= console_log_severity will be written to the debug console as well as
         // the logfile

         // Create entries for all the log levels...
#define BEGIN_GROUP(group)        struct {
#define ADD_CATEGORY(category)         uint32 category;
#define END_GROUP(group)          } group;

         RADIANT_LOG_CATEGORIES

#undef BEGIN_GROUP
#undef ADD_CATEGORY
#undef END_GROUP
      };
   };
};

// _LOG writes to the logfile unconditionally.  This is not the macro
// you're looking for.
#define LOG_(x)  BOOST_LOG_SEV(__radiant_log_source, x)

// Used to write unconditionally to the log for critical messages.  Use
// extremely sparingly.
#define LOG_CRITICAL() LOG_(0)

// Another unconditional logger.  Don't use this unless you've already
// verified the level against some authority!
#define LOG_CATEGORY_(level, prefix) \
   LOG_(level) << " | " << level << " | " << std::setfill(' ') << std::setw(32) << prefix << " | "

// Check to see if the specified log level is enabled
#define LOG_IS_ENABLED(category, level)   (log_levels_.category >= level)

// LOG_CATEGORY writes to the log using a very specialized category string.
// Useful when you want the log to contain context which might change each
// time you execute the log line (e.g. the id of the current object).
#define LOG_CATEGORY(category, level, prefix) if (LOG_IS_ENABLED(category, level)) LOG_CATEGORY_(level, prefix)


// LOG writes to the log if the log level at the specified category is
// greater or equal to the level passed in the macro.  Yes, this means
// LOG(xxx, 0) is logged unconditionally. Don't abuse it!
#define LOG(category, level)  LOG_CATEGORY(category, level, #category)

// Extern variables used by the macros.  Do not reference these directly!
extern struct radiant::logger::LogCategories log_levels_;
extern boost::log::sources::severity_logger< int > __radiant_log_source;

#endif
