#ifndef _RADIANT_PERFMON_STORE_H
#define _RADIANT_PERFMON_STORE_H

#include "namespace.h"

BEGIN_RADIANT_PERFMON_NAMESPACE

class Store {
   public:
      Store();

      Timeline* GetTimeline();

   private:
      std::unique_ptr<Timeline>  timeline_;
};

END_RADIANT_PERFMON_NAMESPACE

#endif // _RADIANT_PERFMON_STORE_H
