#include "radiant.h"
#include "radiant_stdutil.h"
#include "free_motion.h"
#include "nav_grid.h"
#include "physics_util.h"
#include "om/components/mob.ridl.h"

using namespace radiant;
using namespace radiant::phys;

FreeMotion::FreeMotion(NavGrid &ng) :
   _ng(ng)
{
   _guard = _ng.NotifyTileDirty([this](csg::Point3 const& tile) {
      stdutil::UniqueInsert(_dirtyTiles, tile);
   });
}


/*
 * -- FreeMotion::ProcessDirtyTiles
 *
 * Try to unstick all entities on all dirty tiles.  Runs until everyone
 * is unstuck or `t` expires.
 *
 */
void FreeMotion::ProcessDirtyTiles(platform::timer& t)
{
   while (!t.expired() && !_dirtyTiles.empty()) {
      csg::Point3 index = _dirtyTiles.back();
      _dirtyTiles.pop_back();
      ProcessDirtyTile(index);
   }
}

/*
 * -- FreeMotion::ProcessDirtyTile
 *
 * Unstick all entities in the tile specified by `index`.
 *
 */
void FreeMotion::ProcessDirtyTile(csg::Point3 const& index)
{
   _ng.ForEachEntityAtIndex(index, [this](om::EntityPtr e) {
      UnstickEntity(e);
      return false;     // continue iteration through the whole list
   });
}

/*
 * -- FreeMotion::UnstickEntity
 *
 * Checks to see if `entity` is embedded in the ground or floating in the air
 * and takes appropriate action.  Things in the ground are bumped up to the surface.
 * Things in the air fall to the ground.
 *
 */
void FreeMotion::UnstickEntity(om::EntityPtr entity)
{
   om::MobPtr mob = entity->GetComponent<om::Mob>();

   if (mob && mob->GetMobCollisionType() != om::Mob::NONE && !mob->GetInFreeMotion()) {
      // If the position where the entity is located is not standable,
      // do something about it!
      csg::Point3 current = mob->GetWorldGridLocation();
      csg::Point3 valid = _ng.GetStandablePoint(entity, current);

      if (current != valid) {
         if (current.y < valid.y) {
            // The entity is stuck inside a collision shape.  Just pop them
            // up and poke them
            float dy = static_cast<float>(valid.y - current.y);
            mob->MoveTo(mob->GetLocation() + csg::Point3f(0, dy, 0));
         } else {
            // The entity is floating above a collision shape.  Put them into
            // free motion so the simulation can take over.
            mob->SetInFreeMotion(true);
            mob->SetVelocity(csg::Transform(csg::Point3f::zero, csg::Quaternion()));
         }
      }
   }
}

