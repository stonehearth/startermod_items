#include "radiant.h"
#include "radiant_macros.h"
#include "nav_grid.h"
#include "om/components/mob.h"
#include "om/components/terrain.h"
#include "om/components/vertical_pathing_region.h"
#include "om/components/region_collision_shape.h"

using namespace radiant;
using namespace radiant::Physics;


template <class T, typename C>
T ToLocalCoordinates(const T& coord, const std::shared_ptr<C> component)
{
   om::Entity const& entity = component->GetEntity(); 

   auto mob = entity.GetComponent<om::Mob>();
   if (mob) {
      T result(coord);
      auto origin = mob->GetWorldLocation();
      result.Translate(-origin);
      return result;
   }
   return coord;
}

NavGrid::NavGrid() :
   dirty_(false)
{
}

void NavGrid::TrackComponent(dm::ObjectType type, std::shared_ptr<dm::Object> component)
{
   switch (component->GetObjectType()) {
   case om::TerrainObjectType: {
      auto terrain = std::static_pointer_cast<om::Terrain>(component);
      terrain_[terrain->GetObjectId()] = terrain;
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
   regionCollisionShapes_[id] = region;
}

bool NavGrid::CanStand(csg::Point3 const& pt) const
{
   return IsEmpty(pt) && CanStandOn(pt - csg::Point3(0, 1, 0));
}

bool NavGrid::IsEmpty(csg::Point3 const& pt) const
{
   csg::Cube3 bounds(pt, pt + csg::Point3(1, 4, 1));

   {
      auto i = terrain_.begin();
      while (i != terrain_.end()) {
         auto terrain = i->second.lock();
         if (terrain) {
            // xxx: ideally we would iterate through all the zones from the min
            // to the max of the region, but that's very difficult given the
            // terrain interface (e.g. what happens if min is in a zone and max
            // isnt?)  so just check the min..
            csg::Point3 origin;
            om::BoxedRegion3Ptr zone = terrain->GetZone(bounds.GetMin(), origin);
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
         auto region = i->second.lock();
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
   auto region = rgnCollsionShape->GetRegionPtr();
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
      auto vpr = i->second.lock();
      if (vpr) {
         om::BoxedRegion3Ptr r = vpr->GetRegionPtr();
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
   vprs_[id] = vpr;
}
