#ifndef _RADIANT_DM_TRACE_H
#define _RADIANT_DM_TRACE_H

#include "dm.h"

BEGIN_RADIANT_DM_NAMESPACE

class Trace
{
public:
   typedef std::function<void()> ModifiedCb;
   typedef std::function<void()> DestroyedCb;

   Trace(const char* reason, Object const& o, Store const& store);
   virtual ~Trace();

   ObjectId GetObjectId() const;
   Store const& GetStore() const;

   void OnModified(ModifiedCb modified)
   {
      on_modified_ = modified;
   }
   void OnDestroyed(DestroyedCb destroyed)
   {
      on_destroyed_ = destroyed;
   }
   virtual void PushObjectState_() = 0;
   virtual void SaveObjectDelta(Object* obj, Protocol::Value* value) = 0;

   virtual void NotifyDestroyed() { }

public:
   void SignalModified();  // must ALWAYS be signals first when sending multiple signals
   void SignalDestroyed();

protected:
   const char*    reason_;
   Store const&   store_;
   ObjectId       object_id_;
   ModifiedCb     on_modified_;
   DestroyedCb    on_destroyed_;
};

#define FORWARD_BASE_PUSH_OBJECT_STATE() \
   void PushObjectState_() override { \
      PushObjectState(); \
   }

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_TRACE_H

