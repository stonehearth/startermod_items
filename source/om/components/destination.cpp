#include "pch.h"
#include "destination.h"
#include "csg/csg_json.h"

using namespace ::radiant;
using namespace ::radiant::om;

void Destination::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("region", region_);
   AddRecordField("reserved", reserved_);
   AddRecordField("adjacent", adjacent_);
   AddRecordField("auto_update_adjacent", auto_update_adjacent_);

   if (!IsRemoteRecord()) {
      auto_update_adjacent_ = false;
   }
}

/*
 * Looks something like this:
 *
 *     "destination" : {
 *       "region" : [
 *          [[-3, 0, -3], [3, 1, 3]]
 *        ]
 *     }
 */

void Destination::ExtendObject(json::ConstJsonObject const& obj)
{
   if (obj.has("region")) {
      region_ = GetStore().AllocObject<BoxedRegion3>();
      csg::Region3& region = (*region_)->Modify();
      region = obj.get("region", csg::Region3());
   }
   SetAutoUpdateAdjacent(obj.get<bool>("auto_update_adjacent", true));
   LOG(INFO) << "finished constructing new destination for entity " << GetEntity().GetObjectId();
   LOG(INFO) << dm::DbgInfo::GetInfoString(region_);
}

void Destination::SetAutoUpdateAdjacent(bool value)
{
   value = !!value; // cohearse to 1 or 0
   if (auto_update_adjacent_ != value) {
      region_guard_ = nullptr;
      reserved_guard_ = nullptr;
      update_adjacent_guard_.Clear();
      auto_update_adjacent_ = value;

      if (value) {
         dm::ObjectId component_id = GetObjectId();
         auto flush_dirty = [=]() {
            LOG(WARNING) << "(xyz) flushing destination component dirty bit " << component_id << ".";
            UpdateDerivedValues();
            update_adjacent_guard_.Clear();
         };
         auto mark_dirty = [=]() {
            if (update_adjacent_guard_.Empty()) {
               LOG(WARNING) << "(xyz) marking destination component (region|reserved) " << component_id << " as dirty";
               update_adjacent_guard_ = GetStore().TraceFinishedFiringTraces("auto-updating destination region", flush_dirty);
            }
         };
         region_guard_ = TraceBoxedRegion3PtrFieldVoid(region_, "updating destination derived values (region changed)", mark_dirty);
         reserved_guard_ = TraceBoxedRegion3PtrFieldVoid(reserved_, "updating destination derived values (reserved changed)", mark_dirty);
         flush_dirty();
      }
   }
}

void Destination::UpdateDerivedValues()
{
   if (auto_update_adjacent_ && !*adjacent_) {
      adjacent_ = GetStore().AllocObject<BoxedRegion3>();
   }

   if (*region_ && *adjacent_) {
      csg::Region3 const& region = ***region_;
      if (*reserved_) {
         csg::Region3 const& reserved = ***reserved_;
         if (!reserved.IsEmpty()) {
            ComputeAdjacentRegion(region - reserved);
            return;
         }
      }
      ComputeAdjacentRegion(region);
   }
   LOG(WARNING) << "(abc) updated derived values for " << GetEntity();
}

void Destination::ComputeAdjacentRegion(csg::Region3 const& r)
{
   csg::Region3& adjacent = (*adjacent_)->Modify();

   dm::ObjectId component_id = GetObjectId();

   adjacent.Clear();
   for (const csg::Cube3& c : r) {
      csg::Point3 p0 = c.GetMin();
      csg::Point3 p1 = c.GetMax();
      csg::Point3 delta = p1 - p0;
      int x = delta.x, y = delta.y, z = delta.z;

      adjacent.Add(csg::Cube3(p0 - csg::Point3(0, 0, 1), p0 + csg::Point3(x, y, 0)));  // top
      adjacent.Add(csg::Cube3(p0 - csg::Point3(1, 0, 0), p0 + csg::Point3(0, y, z)));  // left
      adjacent.Add(csg::Cube3(p0 + csg::Point3(0, 0, z), p0 + csg::Point3(x, y, z + 1)));  // bottom
      adjacent.Add(csg::Cube3(p0 + csg::Point3(x, 0, 0), p0 + csg::Point3(x + 1, y, z)));  // right
   }
   adjacent.Optimize();
   LOG(WARNING) << "(xyz) destination component " << component_id << " adjacent is now " << adjacent << " " << adjacent_.GetStoreId() << ":" << adjacent_.GetObjectId();
   LOG(WARNING) << "(xyz) adjacent is pointing to " << (*adjacent_)->GetObjectId();
}

void Destination::SetRegion(BoxedRegion3Ptr r)
{
   region_ = r;
}

void Destination::SetReserved(BoxedRegion3Ptr r)
{
   reserved_ = r;
}

void Destination::SetAdjacent(BoxedRegion3Ptr r)
{
   adjacent_ = r;
   LOG(WARNING) << "(xyz) destination component " << GetObjectId() << " adjacent set to " << r << " " << adjacent_.GetStoreId() << ":" << adjacent_.GetObjectId() << " -> " << (r ? r->GetObjectId() : 0);
   LOG(INFO) << dm::DbgInfo::GetInfoString(adjacent_);

   if (auto_update_adjacent_) {
      UpdateDerivedValues();
   }
}
