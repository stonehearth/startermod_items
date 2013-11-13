#include "radiant.h"
#include "radiant_macros.h"
#include "nav_grid.h"
#include "dm/store.h"
#include "om/components/mob.h"
#include "om/components/terrain.h"
#include "om/components/vertical_pathing_region.h"
#include "om/components/region_collision_shape.h"

using namespace radiant;
using namespace radiant::phys;


template <class T, typename C>
T ToLocalCoordinates(const T& coord, const std::shared_ptr<C> component)
{
   om::Entity const& entity = component->GetEntity(); 

   auto mob = entity.GetComponent<om::Mob>();
   if (mob) {
      T result(coord);
      csg::Point3 origin = mob->GetWorldGridLocation();
      result.Translate(-origin);
      return result;
   }
   return coord;
}

NavGrid::NavGrid() :
   next_region_change_cb_id_(1)
{
}

void NavGrid::TrackComponent(dm::ObjectType type, std::shared_ptr<dm::Object> component)
{
   switch (component->GetObjectType()) {
   case om::TerrainObjectType: {
      auto terrain = std::static_pointer_cast<om::Terrain>(component);
      dm::ObjectId id = terrain->GetObjectId();
      terrain_[id] = std::make_shared<TerrainCollisionObject>(terrain);
      break;
   }
   case om::RegionCollisionShapeObjectType: {
      AddRegionCollisionShape(std::static_pointer_cast<om::RegionCollisionShape>(component));
      break;
   }
   case om::VerticalPathingRegionObjectType: {
      AddVerticalPathingRegion(std::static_pointer_cast<om::VerticalPathingRegion>(component));
      break;
   }
   }
}

void NavGrid::AddRegionCollisionShape(om::RegionCollisionShapePtr region)
{
   dm::ObjectId id = region->GetObjectId();
   auto co = std::make_shared<RegionCollisionShapeObject>(region);

   co->guard = om::DeepTraceRegion(region->GetRegion(), "navgrid region tracking", [this](csg::Region3 const& r) {
      FireRegionChangeNotifications(r);
   });
   regionCollisionShapes_[id] = co;
}

bool NavGrid::IsValidStandingRegion(csg::Region3 const& r) const
{
   for (csg::Cube3 const& cube : r) {
      return IsEmpty(cube) && CanStandOn(cube.Translated(csg::Point3(0, -1, 0)));
   }
   return false;
}

bool NavGrid::CanStand(csg::Point3 const& pt) const
{
   return IsEmpty(pt) && CanStandOn(pt - csg::Point3(0, 1, 0));
}

// xxx: we need a "non-walkable" region to clip against.  until that exists,
// do the slow thing...
void NavGrid::ClipRegion(csg::Region3& r) const
{
   csg::Region3 r2;
   for (csg::Cube3 const& cube : r) {
      for (csg::Point3 const& pt : cube) {
         if (CanStand(pt)) {
            r2.AddUnique(pt);
         }
      }
   }
   r = r2;
}

bool NavGrid::IsEmpty(csg::Point3 const& pt) const
{
   csg::Cube3 bounds(pt, pt + csg::Point3(1, 4, 1));
   return IsEmpty(bounds);
}

bool NavGrid::IsEmpty(csg::Cube3 const& bounds) const
{
   {
      auto i = terrain_.begin();
      while (i != terrain_.end()) {
         auto terrain = i->second->obj.lock();
         if (terrain) {
            // xxx: ideally we would iterate through all the zones from the min
            // to the max of the region, but that's very difficult given the
            // terrain interface (e.g. what happens if min is in a zone and max
            // isnt?)  so just check the min..
            csg::Point3 origin;
            om::Region3BoxedPtr zone = terrain->GetZone(bounds.GetMin(), origin);
            if (zone) {
               csg::Region3 const& region = **zone;
               if (region.Intersects(bounds.Translated(-origin))) {
                  return false;
               }
            }
            i++;
         } else {
            i = terrain_.erase(i);
         }
      }
   }

   {
      auto i = regionCollisionShapes_.begin();
      while (i != regionCollisionShapes_.end()) {
         auto region = i->second->obj.lock();
         if (region) {
            if (Intersects(bounds, region)) {
               return false;
            }
            i++;
         } else {
            i = regionCollisionShapes_.erase(i);
         }
      }
   }
   return true;
}


bool NavGrid::Intersects(csg::Cube3 const&  bounds, om::RegionCollisionShapePtr rgnCollsionShape) const
{
   om::Region3BoxedPtr const region = *rgnCollsionShape->GetRegion();
   if (region) {      
      csg::Cube3 box = ToLocalCoordinates(bounds, rgnCollsionShape);
      const csg::Region3& rgn = **region;
      return rgn.Intersects(box);
   }
   return false;
}

bool NavGrid::PointOnLadder(csg::Point3 const& pt) const
{
   auto i = vprs_.begin();
   while (i != vprs_.end()) {
      auto vpr = i->second->obj.lock();
      if (vpr) {
         om::Region3BoxedPtr r = *vpr->GetRegion();
         if (r) {
            csg::Region3 const& region = r->Get();
            csg::Point3 p = ToLocalCoordinates(pt, vpr);
            if (region.Contains(p)) {
               return true;
            }
         }
         i++;
      } else {
         i = vprs_.erase(i);
      }
   }
   return false;
}

// xxx: this is horribly, horribly expensive!  we should do this
// with region clipping or somethign...
bool NavGrid::CanStandOn(csg::Cube3 const& cube) const
{
   csg::Point3 const& min = cube.GetMin();
   csg::Point3 const& max = cube.GetMin();
   csg::Point3 i;

   i.y = min.y - 1;
   for (i.x = min.x; i.x < max.x; i.x++) {
      for(i.z = min.z; i.z < max.z; i.z++) {
         if (!CanStandOn(i)) {
            return false;
         }
      }
   }
   return true;
}

bool NavGrid::CanStandOn(csg::Point3 const& pt) const
{
   // If the blocks in a vertical pathing region, we can
   // totally stand there!
   if (PointOnLadder(pt)) {
      return true;
   }

   // make sure we're on solid ground...
   return !IsEmpty(pt);
}

void NavGrid::AddVerticalPathingRegion(om::VerticalPathingRegionPtr vpr)
{
   dm::ObjectId id = vpr->GetObjectId();
   auto co = std::make_shared<VerticalPathingRegionCollisionObject>(vpr);

   vprs_[id] = co;
}

void NavGrid::FireRegionChangeNotifications(csg::Region3 const& r)
{
   for (const auto& entry : region_change_cb_map_) {
      entry.second.cb();
   }
}

TerrainChangeCbId NavGrid::AddCollisionRegionChangeCb(csg::Region3 const* r, TerrainChangeCb cb)
{
   TerrainChangeCbId id = next_region_change_cb_id_++;
   region_change_cb_map_.insert(std::make_pair(id, TerrainChangeEntry(r, cb)));
   return id;
}

void NavGrid::RemoveCollisionRegionChangeCb(TerrainChangeCbId id)
{
   region_change_cb_map_.erase(id);
}

