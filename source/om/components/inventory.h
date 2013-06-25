#ifndef _RADIANT_OM_INVENTORY_H
#define _RADIANT_OM_INVENTORY_H

#include "math3d.h"
#include "dm/boxed.h"
#include "dm/record.h"
#include "dm/store.h"
#include "dm/map.h"
#include "om/all_object_types.h"
#include "om/om.h"
#include "om/entity.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class Inventory : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(Inventory, inventory);

   static luabind::scope RegisterLuaType(struct lua_State* L, const char* name);

   void SetCapacity(int capacity);
   int GetCapacity() const;
   bool IsFull() const;
   void StashItem(EntityRef item);

   const dm::Map<int, EntityRef>& GetContents() const { return contents_; }
   std::string ToString() const;

private:
   void InitializeRecordFields() override {
      Component::InitializeRecordFields();
      AddRecordField("capacity", capacity_);
      AddRecordField("contents", contents_);
      capacity_ = 0;
   }

public:
   dm::Map<int, EntityRef>    contents_;
   dm::Boxed<int>             capacity_;
};

static std::ostream& operator<<(std::ostream& os, const Inventory& o) { return (os << o.ToString()); }

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_INVENTORY_H
