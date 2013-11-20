#include "pch.h"
#include "destination.ridl.h"
#include "om/region.h"
#include "csg/util.h"

using namespace ::radiant;
using namespace ::radiant::om;

void Destination::ConstructObject()
{
   auto_update_adjacent_ = false;
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

void Destination::ExtendObject(json::Node const& obj)
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
      NOT_YET_IMPLEMENTED(); // UG!
      auto_update_adjacent_ = value;

      if (value) {
         dm::ObjectId component_id = GetObjectId();
         auto flush_dirty = [=]() {
            UpdateDerivedValues();
         };
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
}

void Destination::ComputeAdjacentRegion(csg::Region3 const& r)
{
   (*adjacent_)->Modify() = csg::GetAdjacent(r);
}

Destination& Destination::SetAdjacent(Region3BoxedPtr r)
{
   adjacent_ = r;
   LOG(INFO) << dm::DbgInfo::GetInfoString(adjacent_);

   // Manually setting the adjacent region turns off auto adjacency stuff by default
   SetAutoUpdateAdjacent(false);
   return *this;
}
