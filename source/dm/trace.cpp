#include "radiant.h"
#include "trace.h"
#include "object.h"

using namespace radiant;
using namespace radiant::dm;

Trace::Trace(const char* reason, Object const& o, Store const& store) :
   store_(store),
   object_id_(o.GetObjectId()),
   reason_(reason)
{
}

Trace::~Trace()
{
}

ObjectId Trace::GetObjectId() const
{
   return object_id_;
}

Store const& Trace::GetStore() const
{
   return store_;
}   

void Trace::SignalModified()
{
   if (on_modified_) {
      on_modified_();
   }
}

void Trace::SignalDestroyed()
{
   if (on_destroyed_) {
      on_destroyed_();
   }
}
