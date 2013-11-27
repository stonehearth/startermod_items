#ifndef _RADIANT_DM_TRACE_IMPL_H
#define _RADIANT_DM_TRACE_IMPL_H

#include "trace.h"

BEGIN_RADIANT_DM_NAMESPACE

template <typename Derived>
class TraceImpl : public Trace,
                  public std::enable_shared_from_this<Derived>
{
public:
   TraceImpl(const char* reason) :
      Trace(reason)
   {
   }

   std::shared_ptr<Derived> OnModified(ModifiedCb modified)
   {
      Trace::OnModified_(modified);
      return shared_from_this();
   }

   std::shared_ptr<Derived> OnDestroyed(DestroyedCb destroyed)
   {
      Trace::OnDestroyed_(destroyed);
      return shared_from_this();
   }

   std::shared_ptr<Derived> PushObjectState()
   {
      PushObjectState_();
      return shared_from_this();
   }
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_TRACE_IMPL_H

