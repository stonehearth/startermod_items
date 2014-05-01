#ifndef _RADIANT_DM_TRACE_H
#define _RADIANT_DM_TRACE_H

#include <functional>
#include "dm.h"
#include "core/object_counter.h"

BEGIN_RADIANT_DM_NAMESPACE

class Trace : public core::ObjectCounter<Trace>
{
public:
   typedef std::function<void()> ModifiedCb;
   typedef std::function<void()> DestroyedCb;

   Trace(const char* reason);
   virtual ~Trace();

   void OnModified_(ModifiedCb modified)
   {
      on_modified_ = modified;
   }
   void OnDestroyed_(DestroyedCb destroyed)
   {
      on_destroyed_ = destroyed;
   }
   
   void PushObjectState_()
   {
      ClearCachedState();
      SignalObjectState();
   }

   virtual void NotifyDestroyed() { }

protected:
   virtual void SignalObjectState() = 0;
   virtual void ClearCachedState() { }
   std::string const& GetReason() const;

public:
   void SignalModified();  // must ALWAYS be signals first when sending multiple signals
   void SignalDestroyed();

protected:
   std::string    reason_;
   ModifiedCb     on_modified_;
   DestroyedCb    on_destroyed_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_TRACE_H

