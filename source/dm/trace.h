#ifndef _RADIANT_DM_TRACE_H
#define _RADIANT_DM_TRACE_H

#include "dm.h"

BEGIN_RADIANT_DM_NAMESPACE

class Trace
{
public:
   typedef std::function<void()> ModifiedCb;
   typedef std::function<void()> DestroyedCb;

public:
   Trace(const char* reason) : reason_(reason) { }
   virtual ~Trace() { }

   void OnModified(ModifiedCb modified)
   {
      on_modified_ = modified;
   }

   void OnDestroyed(DestroyedCb destroyed)
   {
      on_destroyed_ = destroyed;
   }

protected:
   void SignalModified()
   {
      if (on_modified_) {
         on_modified_();
      }
   }

   void SignalDestroyed()
   {
      if (on_destroyed_) {
         on_destroyed_();
      }
   }

private:
   const char*    reason_;
   ModifiedCb     on_modified_;
   DestroyedCb    on_destroyed_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_TRACE_H

