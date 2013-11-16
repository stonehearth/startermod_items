#ifndef _RADIANT_DM_TRACKER_H
#define _RADIANT_DM_TRACKER_H

#include "namespace.h"

BEGIN_RADIANT_DM_NAMESPACE

class Tracker {
public:
   Tracker(const char* reason) : 
      reason_(reason)
   {
   }

   virtual ~Tracker() {
   }
}

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_TRACKER_H

