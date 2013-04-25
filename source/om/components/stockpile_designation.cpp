#include "pch.h"
#include "stockpile_designation.h"
#include "entity_container.h"
#include "mob.h"
#include "om/stonehearth.h"

using namespace ::radiant;
using namespace ::radiant::om;

StockpileDesignation::StockpileDesignation()
{
}

StockpileDesignation::~StockpileDesignation()
{
}

static void ReserveAdjacent(lua_State* L, StockpileDesignation& s, math3d::ipoint3 location)
{
   math3d::ipoint3 block;

   bool reserved = s.ReserveAdjacent(location, block);
   luabind::object(L, reserved).push(L);
   luabind::object(L, block).push(L);
}

luabind::scope StockpileDesignation::RegisterLuaType(struct lua_State* L, const char* name)
{
   using namespace luabind;
   return
      class_<StockpileDesignation, Component, std::weak_ptr<Component>>(name)
         .def(tostring(self))
         .def("is_full",               &om::StockpileDesignation::IsFull)
         .def("get_bounds",            &om::StockpileDesignation::GetBounds)
         .def("set_bounds",            &om::StockpileDesignation::SetBounds)
         .def("set_container",         &om::StockpileDesignation::SetContainer)
         .def("reserve_adjacent",      &::ReserveAdjacent)
         .def("get_available_region",  &om::StockpileDesignation::GetAvailableRegion)
         .def("get_standing_region",   &om::StockpileDesignation::GetStandingRegion)
         .def("get_contents",          &om::StockpileDesignation::GetContents)
      ;
}

std::string StockpileDesignation::ToString() const
{
   std::ostringstream os;
   os << "(StockpileDesignation id:" << GetObjectId() << " containing:" << contents_.Size() << ")";
   return os.str();
}

void StockpileDesignation::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("bounds", bounds_);
   AddRecordField("available", available_);
   AddRecordField("reserved", reserved_);
   AddRecordField("contents", contents_);
   AddRecordField("standingRgn", standingRgn_);
}

bool StockpileDesignation::ReserveAdjacent(const math3d::ipoint3& pt, math3d::ipoint3 &reserved)
{
   static const csg::Point3 adjacent[] = {
      csg::Point3( 1,  0,  0),
      csg::Point3(-1,  0,  0),
      csg::Point3( 0,  0, -1),
      csg::Point3( 0,  0,  1),
   };
   const csg::Point3 origin = GetWorldOrigin();
   const csg::Point3 standing = pt - origin;

   csg::Region3 const& available = GetAvailableRegion();
   for (const auto& offset : adjacent) {
      const csg::Point3 block = standing + offset;
      if (available.Contains(block)) {
         reserved = block + origin;
         available_.Modify() -= block;
         reserved_.Modify() += block;
         UpdateStandingRegion();
         return true;
      }
   }
   return false;
}

void StockpileDesignation::SetContainer(EntityContainerRef c)
{
   auto containter = c.lock();
   if (containter) {
      auto added = std::bind(&StockpileDesignation::AddEntity, this, std::placeholders::_1, std::placeholders::_2);
      auto removed = std::bind(&StockpileDesignation::RemoveEntity, this, std::placeholders::_1);
      traces_ += containter->GetChildren().TraceMapChanges("stockpile contents", added, removed);
   }
}

void StockpileDesignation::AddEntity(dm::ObjectId id, EntityRef e)
{
   bool changed = false;
   auto entity = e.lock();
   if (entity) {
      auto item = entity->GetComponent<om::Item>();
      if (item) {
         auto mob = entity->GetComponent<om::Mob>();
         if (mob) {
            math3d::ipoint3 origin = GetWorldOrigin();
            math3d::ipoint3 pt = mob->GetWorldGridLocation() - origin;
            if ((*bounds_).contains(pt)) {
               id = entity->GetEntityId();

               itemLocations_[id] = pt;
               contents_.Insert(id);

               available_.Modify() -= pt;
               reserved_.Modify() -= pt;
               changed = true;
            }
         }
      }
   }
   if (changed) {
      UpdateStandingRegion();
   }
}

void StockpileDesignation::RemoveEntity(om::EntityId id)
{
   auto i = itemLocations_.find(id);
   if (i != itemLocations_.end()) {
      contents_.Remove(id);
      available_.Modify() += i->second;
      itemLocations_.erase(i);
      UpdateStandingRegion();
   }
}

const csg::Region3& StockpileDesignation::GetAvailableRegion() const
{
   return *available_;
}

// Currently assumes (and ASSERTs verify) that this only gets called
// once, before the container is set.
void StockpileDesignation::SetBounds(const math3d::ibounds3& bounds)
{
   bounds_ = bounds;

   ASSERT(bounds._max.y == bounds._min.y + 1);
   ASSERT((*reserved_).IsEmpty());
   ASSERT((*available_).IsEmpty());
   ASSERT(!(*standingRgn_));

   csg::Region3 available = csg::Region3(bounds);
   available_ = available;

   standingRgn_ = GetStore().AllocObject<Region>();
   UpdateStandingRegion();
}

// Not actually "fully" but, fully committed to being full.  There may
// be empty spaces in the stockpile which workers are on their way
// toward filling.
bool StockpileDesignation::IsFull() const
{
   return (*available_).IsEmpty();
}

void StockpileDesignation::UpdateStandingRegion()
{
   csg::Region3& standing = (*standingRgn_)->Modify();
   standing = Stonehearth::ComputeStandingRegion(*available_ - *reserved_, 1);
   standing.Translate(GetWorldOrigin());
}
