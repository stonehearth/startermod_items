#include "pch.h"
#include "fixture.h"
#include "entity_container.h"
#include "mob.h"
#include "om/stonehearth.h"

using namespace ::radiant;
using namespace ::radiant::om;

Fixture::Fixture() :
   BuildOrder()
{
   LOG(WARNING) << "creating new fixture...";
}

void Fixture::InitializeRecordFields()
{
   BuildOrder::InitializeRecordFields();
   AddRecordField("item",     item_);
   AddRecordField("reserved", reserved_);
   AddRecordField("kind",     kind_);
   AddRecordField("standing", standingRegion_);
}

bool Fixture::NeedsMoreWork()
{
   return !GetItem().lock();
}

const RegionPtr Fixture::GetConstructionRegion() const
{
   return *standingRegion_;
}

bool Fixture::ReserveAdjacent(const math3d::ipoint3& pt, math3d::ipoint3 &reserved)
{
   if (*reserved_) {
      return false;   
   }
   reserved_ = true;
   reserved = GetEntity().GetComponent<Mob>()->GetGridLocation();
   return true;
}

void Fixture::ConstructBlock(const math3d::ipoint3& block)
{
   ASSERT(false); // should not call ConstructBlock on a fixture.  Call SetItem!
}

void Fixture::StartProject(const dm::CloneMapping& mapping)
{
   standingRegion_ = GetStore().AllocObject<Region>();
   csg::Region3& rgn = (*standingRegion_)->Modify();

   math3d::ipoint3 origin = GetWorldOrigin();
   rgn += csg::Point3( 1, 0,  0) + origin;
   rgn += csg::Point3(-1, 0,  0) + origin;
   rgn += csg::Point3( 0, 0,  1) + origin;
   rgn += csg::Point3( 0, 0, -1) + origin;

   item_ = nullptr;

   reserved_ = false;
}

void Fixture::CompleteTo(int percent)
{
   if (percent >= 100) {
      auto ec = GetEntity().AddComponent<EntityContainer>();
      if (ec->GetChildren().GetSize() == 0) {
         EntityPtr item = Stonehearth::CreateEntity(GetStore(), GetItemKind());
         SetItem(item);
      }
   }
}

dm::Guard Fixture::TraceRegion(const char* reason, std::function<void(const csg::Region3&)> cb) const
{
   return dm::Guard();
}

dm::Guard Fixture::TraceReserved(const char* reason, std::function<void(const csg::Region3&)> cb) const
{
   return dm::Guard();
}

const csg::Region3& Fixture::GetRegion() const
{
   return csg::Region3::empty;
}

const csg::Region3& Fixture::GetReservedRegion() const
{
   return csg::Region3::empty;
}

void Fixture::SetItem(EntityRef item)
{
   item_ = item.lock();
   if (*item_) {
      GetEntity().AddComponent<EntityContainer>()->AddChild(item);
   }
}

