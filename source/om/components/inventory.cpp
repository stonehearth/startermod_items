#include "pch.h"
#include "inventory.h"

using namespace ::radiant;
using namespace ::radiant::om;

luabind::scope Inventory::RegisterLuaType(struct lua_State* L, const char* name)
{
   using namespace luabind;

   std::string contentsName = std::string(name) + "Contents";

   return
      class_<Inventory, Component, std::weak_ptr<Component>>(name)
         .def(tostring(self))
         .def("set_capacity",    &om::Inventory::SetCapacity)
         .def("get_capacity",    &om::Inventory::GetCapacity)
         .def("is_full",         &om::Inventory::IsFull)
         .def("get_contents",    &om::Inventory::GetContents)
         .def("stash_item",      &om::Inventory::StashItem)
      ;
}

std::string Inventory::ToString() const
{
   ostringstream os;
   os << "(Inventory id:" << GetObjectId() << " capacity:" << *capacity_ << " holding:" << contents_.GetSize() << ")";
   return os.str();
}

int Inventory::GetCapacity() const
{
   return *capacity_;
}

void Inventory::SetCapacity(int capacity)
{
   capacity_ = capacity;
}

bool Inventory::IsFull() const
{
   // xxx: need to check for destroyed entities to make
   // room...
   return contents_.GetSize() >= capacity_;
}

void Inventory::StashItem(EntityRef item)
{
   if (!IsFull()) {
      for (int i = 0; i < capacity_; i++) {
         auto entity = contents_[i].lock();
         if (!entity) {
            contents_[i] = item;
            return;
         }
      }
   }
}

