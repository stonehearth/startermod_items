#ifndef _RADIANT_METRICS_PROFILE_H
#define _RADIANT_METRICS_PROFILE_H

#if 1 // defined(DEBUG)
#  define PROFILE_METRICS()       radiant::metrics::store::get_current_thread_store()
#  define PROFILE_RESET()         PROFILE_METRICS().reset()
#  define PROFILE_DUMP_STATS()    PROFILE_METRICS().dump_stats();
#  define PROFILE_BLOCK()         metrics::profile::counter profile_block_var(PROFILE_METRICS(), __FUNCTION__)
#  define PROFILE_BLOCK_NAME(c)   metrics::profile::counter profile_block_var(PROFILE_METRICS(), __FUNCTION__ ## " " ## #c)
#  define PROFILE_BLOCK_STR(c)    metrics::profile::counter profile_block_var(PROFILE_METRICS(), c)
#else
// xxx: NOT acceptable!  We still want metrics in release builds.  Need to either find a way to
// make collecting profile metrics much much much faster, or filtering some out of release builds.
// ideally both.
#  define PROFILE_METRICS()
#  define PROFILE_RESET()
#  define PROFILE_DUMP_STATS()
#  define PROFILE_BLOCK()
#  define PROFILE_BLOCK_NAME(c)
#endif

namespace radiant {
   namespace metrics {
      class store;
      namespace profile {
         class counter {
            public:
               counter(metrics::store &store, const char *category);
               counter(metrics::store &store, std::string &category);
               ~counter();

            protected:
               store                   &_store;
               metrics::time           _start;
         };
      }
   };
};
#endif // _RADIANT_METRICS_PROFILE_H
