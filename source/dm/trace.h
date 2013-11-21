#ifndef _RADIANT_DM_TRACE_H
#define _RADIANT_DM_TRACE_H

#include "dm.h"

BEGIN_RADIANT_DM_NAMESPACE

class Trace
{
public:
   virtual ~Trace();
};

template <typename Derived>
class TraceImpl : public Trace,
                  public std::enable_shared_from_this<Derived>
{
public:
   typedef std::function<void()> ModifiedCb;
   typedef std::function<void()> DestroyedCb;

public:
   TraceImpl(const char* reason, Object const& o, Store const& store) : store_(store), object_id_(o.GetObjectId()), reason_(reason) { }
   virtual ~TraceImpl() { }

   std::shared_ptr<Derived> OnModified(ModifiedCb modified)
   {
      on_modified_ = modified;
      return shared_from_this();
   }

   std::shared_ptr<Derived> OnDestroyed(DestroyedCb destroyed)
   {
      on_destroyed_ = destroyed;
      return shared_from_this();
   }

   ObjectId GetObjectId() const
   {
      return object_id_;
   }

   Store const& GetStore() const
   {
      return store_;
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
   Store const&   store_;
   ObjectId       object_id_;
   ModifiedCb     on_modified_;
   DestroyedCb    on_destroyed_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_TRACE_H

