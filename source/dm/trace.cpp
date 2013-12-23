#include "radiant.h"
#include "trace.h"
#include "object.h"

using namespace radiant;
using namespace radiant::dm;

Trace::Trace(const char* reason) :
   reason_(reason)
{
}

Trace::~Trace()
{
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

std::string const& Trace::GetReason() const
{
   return reason_;
}