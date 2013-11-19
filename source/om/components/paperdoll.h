#ifndef _RADIANT_OM_PAPER_DOLL_H
#define _RADIANT_OM_PAPER_DOLL_H

#include "dm/boxed.h"
#include "dm/record.h"
#include "dm/store.h"
#include "dm/array.h"
#include "om/object_enums.h"
#include "om/om.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class Paperdoll : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(Paperdoll, paperdoll);

   enum Slot {
      MAIN_HAND         = 0,
      WEAPON_SCABBARD,
      NUM_SLOTS,
   };

   dm::Array<EntityRef, NUM_SLOTS> const& GetSlots() const { return slots_; }
   EntityRef GetItemInSlot(Slot slot) const { return slots_[slot]; }
   bool HasItemInSlot(Slot slot) const { return slots_[slot].lock(); }
   void SetSlot(int slot, EntityRef e) { slots_.Set(slot, e); }
   void ClearSlot(int slot) { slots_.Set(slot, EntityRef()); }

private:
   void InitializeRecordFields() override;

public:
   dm::Array<EntityRef, NUM_SLOTS> slots_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_PAPERDOLL_H
