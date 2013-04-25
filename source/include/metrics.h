#ifndef _RADIANT_METRICS_H
#define _RADIANT_METRICS_H

namespace radiant {
   namespace metrics {
      typedef __int64 time;

      inline void get_time(time &p) {
         QueryPerformanceCounter((LARGE_INTEGER *)&p);
      }
   };
};

#include "metrics/store.h"
#include "metrics/profile.h"

#endif // _RADIANT_METRICS_H
