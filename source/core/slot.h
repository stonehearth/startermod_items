#ifndef _RADIANT_CORE_SLOT_H
#define _RADIANT_CORE_SLOT_H

#include <vector>
#include "guard.h"

// Named after the classic signal/slot pattern...
// The client must assure that the owner of the slot out-lives all
// the clients!

BEGIN_RADIANT_CORE_NAMESPACE

// a better implementation with variadic templates would be nice..
template <typename A0>
class Slot
{
public:
   typedef std::function<void(A0 const&)> Fn;

   Slot() : firing_(false) { }

   core::Guard Register(Fn fn) {
      int id = next_id_++;
   
      if (firing_) {
         added_callbacks_[id] = fn;
      } else {
         callbacks_[id] = fn;
      }
      return core::Guard([this, id]() {
         Remove(id);
      });
   }

   void Signal(A0 const& arg) {
      firing_ = true;
      for (auto const& entry : callbacks_) {
         entry.second(arg);
      }
      for (int id : removed_callbacks_) {
         callbacks_.erase(id);
      }
      callbacks_.insert(added_callbacks_.begin(), added_callbacks_.end());
      added_callbacks_.clear();
      removed_callbacks_.clear();
      firing_ = false;
   }

private:
   void Remove(int id) {
      if (firing_) {
         auto i = added_callbacks_.find(id);
         if (i == added_callbacks_.end()) {
            removed_callbacks_.push_back(id);
         } else {
            added_callbacks_.erase(i);
         }
      } else {
         callbacks_.erase(id);
      }
   }

private:
   NO_COPY_CONSTRUCTOR(Slot);

private:
   bool                             firing_;
   int                              next_id_;
   std::unordered_map<int,Fn>       callbacks_;
   std::vector<int>                 removed_callbacks_;
   std::unordered_map<int,Fn>       added_callbacks_;
};

END_RADIANT_CORE_NAMESPACE

#endif // _RADIANT_CORE_SLOT_H
