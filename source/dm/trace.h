#ifndef _RADIANT_DM_TRACE_H
#define _RADIANT_DM_TRACE_H

#include "dm.h"

BEGIN_RADIANT_DM_NAMESPACE

class Trace
{
public:
   typedef std::function<void()> ChangedCb;

public:
   Trace(const char* reason) : reason_(reason) { }
   virtual ~Trace() { }

   void OnChanged(ChangedCb changed)
   {
      on_changed_ = changed;
   }

protected:
   void SignalChanged()
   {
      if (on_changed_) {
         on_changed_();
      }
   }

private:
   const char*    reason_;
   ChangedCb      on_changed_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_TRACE_H

