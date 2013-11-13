#ifndef _RADIANT_CORE_BUFFERED_SLOT_H
#define _RADIANT_CORE_BUFFERED_SLOT_H

#include <vector>
#include "slot.h"

// Named after the classic signal/slot pattern...

BEGIN_RADIANT_CORE_NAMESPACE

template <typename A0>
class BufferedSlot : public Slot<A0>
{
public:
   BufferedSlot(std::string const& name) : Slot(name), last_state_set_(false) { }
   BufferedSlot(std::string const& name, A0 const& a0) : Slot(name), last_state_(a0), last_state_set_(false) { }

   void Signal(A0 const& arg0) {
      last_state_ = arg0;
      last_state_set_ = true;
      Slot::Signal(arg0);
   }

   core::Guard Register(typename Slot::Fn fn) {
      if (last_state_set_) {
         fn(last_state_);
      }
      return Slot::Register(fn);
   }

private:
   A0       last_state_;
   bool     last_state_set_;
};

END_RADIANT_CORE_NAMESPACE

#endif // _RADIANT_CORE_BUFFERED_SLOT_H
