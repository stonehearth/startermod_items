#ifndef _RADIANT_DM_TRACE_H
#define _RADIANT_DM_TRACE_H

#include "dm.h"

BEGIN_RADIANT_DM_NAMESPACE

class Trace
{
public:
   Trace(const char* reason) : 
      reason_(reason)
   {
   }

   virtual ~Trace() {
   }

private:
   const char*    reason_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_TRACE_H

