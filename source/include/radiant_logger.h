#ifndef _RADIANT_LOGGER_H
#define _RADIANT_LOGGER_H

// Use the static libray version
#define GFLAGS_DLL_DECL
#define GFLAGS_DLL_DECLARE_FLAG
#define GFLAGS_DLL_DEFINE_FLAG
#define GOOGLE_GLOG_DLL_DECL

#if !defined(SWIG)
#  pragma warning(push)
#    pragma warning(disable: 4244)
#    include <glog/logging.h>
#  pragma warning(pop)
#endif

namespace radiant {
   namespace logger {
      void init();
      void exit();
   };
   void log(char *str);
};

#endif // _RADIANT_LOGGER_H
