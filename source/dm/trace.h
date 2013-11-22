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

template <typename Derived>
class TraceImpl : public Trace,
                  public std::enable_shared_from_this<Derived>
{
public:
   TraceImpl(const char* reason, Object const& o, Store const& store) :
      Trace(reason, o, store)
   {
   }

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
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_TRACE_H

