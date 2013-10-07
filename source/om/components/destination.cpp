#include "pch.h"
#include "destination.h"
#include "csg/csg_json.h"
#include "csg/util.h"

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
      region_ = GetStore().AllocObject<Region3Boxed>();
      csg::Region3& region = (*region_)->Modify();
      region = obj.get("region", csg::Region3());
   }
   SetAutoUpdateAdjacent(obj.get<bool>("auto_update_adjacent", true));
   LOG(INFO) << "finished constructing new destination for entity " << GetEntity().GetObjectId();
   LOG(INFO) << dm::DbgInfo::GetInfoString(region_);
}

Destination& Destination::SetAutoUpdateAdjacent(bool value)
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
         region_guard_ = DeepTraceRegionVoid(region_, "updating destination derived values (region changed)", mark_dirty);
         reserved_guard_ = DeepTraceRegionVoid(reserved_, "updating destination derived values (reserved changed)", mark_dirty);
         mark_dirty();
      }
   }
   return *this;
}

void Destination::UpdateDerivedValues()
{
   if (auto_update_adjacent_ && !*adjacent_) {
      adjacent_ = GetStore().AllocObject<Region3Boxed>();
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
   (*adjacent_)->Modify() = csg::GetAdjacent(r);

   LOG(WARNING) << "(xyz) destination component " << GetObjectId() << " adjacent is now ... " << adjacent_.GetStoreId() << ":" << adjacent_.GetObjectId();
   LOG(WARNING) << "(xyz) adjacent is pointing to " << (*adjacent_)->GetObjectId();
}

void Destination::SetRegion(Region3BoxedPtr r)
{
   region_ = r;
}

void Destination::SetReserved(Region3BoxedPtr r)
{
   reserved_ = r;
}

void Destination::SetAdjacent(Region3BoxedPtr r)
{
   adjacent_ = r;
   LOG(WARNING) << "(xyz) destination component " << GetObjectId() << " adjacent set to " << r << " " << adjacent_.GetStoreId() << ":" << adjacent_.GetObjectId() << " -> " << (r ? r->GetObjectId() : 0);
   LOG(INFO) << dm::DbgInfo::GetInfoString(adjacent_);

   // Manually setting the adjacent region turns off auto adjacency stuff by default
   SetAutoUpdateAdjacent(false);
}
