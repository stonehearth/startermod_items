#include "pch.h"
#include "destination.ridl.h"
#include "om/region.h"
#include "csg/util.h"

using namespace ::radiant;
using namespace ::radiant::om;

#define D_LOG(level)    LOG(om.destination, level)

std::ostream& operator<<(std::ostream& os, const Destination& o)
{
   os << "[Destination]";
   return os;
}

void Destination::ConstructObject()
{
   Component::ConstructObject();
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

void Destination::LoadFromJson(json::Node const& obj)
{
   if (obj.has("region")) {
      region_ = GetStore().AllocObject<Region3Boxed>();
      (*region_)->Set(obj.get("region", csg::Region3()));
   }
   if (obj.has("adjacent")) {
      adjacent_ = GetStore().AllocObject<Region3Boxed>();
      (*adjacent_)->Set(obj.get("adjacent", csg::Region3()));
   }
   bool dflt = *region_ != nullptr && *adjacent_ == nullptr;
   auto_update_adjacent_ =  obj.get<bool>("auto_update_adjacent", dflt);
   allow_diagonal_adjacency_ = obj.get<bool>("allow_diagonal_adjacency", false);

   D_LOG(5) << "finished constructing new destination for entity " << GetEntity().GetObjectId();
}


void Destination::SerializeToJson(json::Node& node) const
{
   Component::SerializeToJson(node);

   om::Region3BoxedPtr region = GetRegion();
   if (region) {
      node.set("region", region->Get());
   }
   om::Region3BoxedPtr adjacent = GetAdjacent();
   if (adjacent) {
      node.set("adjacent", adjacent->Get());
   }
   node.set("auto_update_adjacent", GetAutoUpdateAdjacent());
   node.set("allow_diagonal_adjacency", GetAllowDiagonalAdjacency());
}

Destination& Destination::SetAutoUpdateAdjacent(bool value)
{
   value = !!value; // cohearse to 1 or 0
   if (auto_update_adjacent_ != value) {
      auto_update_adjacent_ = value;
      OnAutoUpdateAdjacentChanged();
   }
   return *this;
}

void Destination::OnAutoUpdateAdjacentChanged()
{
   if (*auto_update_adjacent_ && !region_trace_) {
      dm::ObjectId component_id = GetObjectId();
      region_trace_ = TraceRegion("auto_update_adjacent", dm::OBJECT_MODEL_TRACES)
         ->OnChanged([this](csg::Region3 const& r) {
            UpdateDerivedValues();
         });

      reserved_trace_ = TraceReserved("auto_update_adjacent", dm::OBJECT_MODEL_TRACES)
         ->OnChanged([this](csg::Region3 const& r) {
            UpdateDerivedValues();
         });

      UpdateDerivedValues();
   } else {
      region_trace_ = nullptr;
      reserved_trace_ = nullptr;
   }
}

Destination& Destination::SetAllowDiagonalAdjacency(bool value)
{
   value = !!value; // cohearse to 1 or 0
   if (allow_diagonal_adjacency_ != value) {
      allow_diagonal_adjacency_ = value;
      if (auto_update_adjacent_) {
         UpdateDerivedValues();
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
   ASSERT(*adjacent_);
   (*adjacent_)->Set(csg::GetAdjacent(r, allow_diagonal_adjacency_, 0, 0));
}

Destination& Destination::SetAdjacent(Region3BoxedPtr r)
{
   adjacent_ = r;

   // Manually setting the adjacent region turns off auto adjacency stuff by default
   SetAutoUpdateAdjacent(false);
   return *this;
}

void Destination::Initialize()
{
   OnAutoUpdateAdjacentChanged();
}

csg::Point3 Destination::GetBestPointOfInterest(csg::Region3 const& r, csg::Point3 const& pt) const
{
   csg::Point3 poi = r.GetClosestPoint(pt);

   // The point of interest should not be the same as the requested point.  If
   // possible, find another one in the region which is adjacent to this point
   if (poi == pt) {
      static const csg::Point3 adjacent[] = {
         csg::Point3(-1, 0,  0),
         csg::Point3( 1, 0,  0),
         csg::Point3( 0, 0, -1),
         csg::Point3( 0, 0, -1),
      };
      for (csg::Point3 const& delta : adjacent) {
         csg::Point3 candidate = poi + delta;
         if (r.Contains(candidate)) {
            return candidate;
         }
      }
   }

   return poi;
}

csg::Point3 Destination::GetPointOfInterest(csg::Point3 const& pt) const
{
   if (!*region_) {
      throw std::logic_error("destination has no region in GetPointOfInterest");
   }
   csg::Region3 const& rgn = ***region_;
   if (*reserved_) {
      csg::Region3 const& reserved = ***reserved_;
      if (!reserved.IsEmpty()) {
         csg::Region3 r = (rgn - reserved);
         if (r.IsEmpty()) {
            D_LOG(1) << "region is empty in get point of interest!";
            return csg::Point3(0, 0, 0);
         }
         return GetBestPointOfInterest(r, pt);
      }
   }
   if (rgn.IsEmpty()) {
      D_LOG(1) << "region is empty in get point of interest!";
      return csg::Point3(0, 0, 0);
   }
   return GetBestPointOfInterest(rgn, pt);
}

