#ifndef _RADIANT_LOGGER_H
#define _RADIANT_LOGGER_H

#include <boost/log/core.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/filesystem.hpp>

namespace radiant {
   namespace logger {
      enum Severity {
         INFO = 0,
         WARNING = 1,
         ERROR = 2
      };
      void init(boost::filesystem::path const& logfile);
      void flush();
      void exit();
   };
};

extern boost::log::sources::severity_logger< ::radiant::logger::Severity > __radiant_log_source;

#define LOG(x)  BOOST_LOG_SEV(__radiant_log_source, ::radiant::logger::x)

#endif // _RADIANT_LOGGER_H
