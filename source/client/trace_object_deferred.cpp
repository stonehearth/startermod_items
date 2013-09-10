#include "pch.h"
#include "mouse_event_promise.h"

using namespace ::radiant;
using namespace ::radiant::client;

TraceObjectDeferred::TraceObjectDeferred(std::shared_ptr<dm::Object> obj, const char* reason) :
   reason_(reason)
{
   Reset(obj);
}

TraceObjectDeferred::~TraceObjectDeferred()
{
}

void TraceObjectDeferred::Reset(dm::ObjectPtr obj)
{
   if (obj) {
      dm::ObjectRef o = obj;
      guard_ = obj->TraceObjectChanges(reason_.c_str(), [o, this]() {
         auto obj = o.lock();
         if (obj) {
            Notify(obj);
         }
      });
      Notify(obj);
   } else {
      guard_.Clear();
   }
}
