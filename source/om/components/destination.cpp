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

   if (!IsRemoteRecord()) {
      region_ = GetStore().AllocObject<BoxedRegion3>();
      reserved_ = GetStore().AllocObject<BoxedRegion3>();
      adjacent_ = GetStore().AllocObject<BoxedRegion3>();
      lastUpdated_ = 0;

      auto update = [=]() {
         UpdateDerivedValues();
      };

      guards_ += (*region_)->TraceObjectChanges("updating destination derived values (region changed)", update);
      guards_ += (*reserved_)->TraceObjectChanges("updating destination derived values (reserved changed)", update);
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
      csg::Region3& region = (*region_)->Modify();
      region += obj.get("region", csg::Region3());
   }
}

void Destination::UpdateDerivedValues()
{
   // This serves multiple purposes... all of them quite annoying.
   // 1) Occasionally, a client asks for the adjacent region after modifiying
   //    the actual region, but before traces have had a chance to fire.  For
   //    instance, this happens when a callback from the solved function of a
   //    multi-pathfinder modifies a shared destination.  In those cases, we
   //    need to calcaulate a new adjance region on request, which is why this
   //    function is called from Destination::GetAdjacent().
   // 2) We only want to recompute the value of the adjacent region when something
   //    changes, so keep track of the last time we modified it and only update if it's
   //    out of date.  Even if we weren't doing 1, we would still have to do this as 
   //    multiple traces could fire if we changed region and reserved since the last
   //    time we updated adjacent.
   int lastModified = std::max((*region_)->GetLastModified(), (*reserved_)->GetLastModified());

   if (lastModified > (*adjacent_)->GetLastModified()) {
      csg::Region3 const& region = ***region_;
      csg::Region3 const& reserved = ***reserved_;
      if (reserved.IsEmpty()) {
         ComputeAdjacentRegion(region);
      } else {
         ComputeAdjacentRegion(region - reserved);
      }
   }
}

void Destination::ComputeAdjacentRegion(csg::Region3 const& r)
{
   csg::Region3& adjacent = (*adjacent_)->Modify();

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
}

BoxedRegion3Ptr Destination::GetReserved() const
{
   return reserved_;
}

BoxedRegion3Ptr Destination::GetAdjacent()
{
   UpdateDerivedValues();
   return adjacent_;
}

