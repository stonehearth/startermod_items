#include "radiant.h"
#include "radiant_stdutil.h"
#include "free_motion.h"
#include "nav_grid.h"
#include "physics_util.h"
#include "csg/util.h"
#include "om/components/mob.ridl.h"
#include "simulation/simulation.h"

using namespace radiant;
using namespace radiant::phys;

FreeMotion::FreeMotion(NavGrid &ng) :
   _ng(ng)
{
   _guard = _ng.NotifyTileDirty([this](csg::Point3 const& index) {
      LOG(physics.free_motion, 4) << "Adding tile " << index << " to dirty set.";
      stdutil::UniqueInsert(_dirtyTiles, index);
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
   LOG(physics.free_motion, 4) << "Unsticking entities on tile " << index;
   _ng.ForEachEntityAtIndex(index, [this](om::EntityPtr e) {
      if (e->GetObjectId() != 1) {
         UnstickEntity(e);
      }
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

   LOG(physics.free_motion, 4) << "Unsticking entity " << *entity;


   if (mob && !mob->GetInFreeMotion() && !mob->GetIgnoreGravity()) {
      om::Mob::MobCollisionTypes type = mob->GetMobCollisionType();

      if (type != om::Mob::NONE) {
         // If the position where the entity is located is not standable,
         // do something about it!
         om::EntityRef entityRoot;
         csg::Point3 current = csg::ToClosestInt(mob->GetWorldGridLocation(entityRoot));
         csg::Point3 valid = _ng.GetStandablePoint(mob->GetEntityPtr(), current);

         if (current != valid) {
            switch (type) {
            case om::Mob::TINY:
            case om::Mob::HUMANOID:
            case om::Mob::TITAN:
               if (current.y < valid.y) {
                  // The entity is stuck inside a collision shape.  Just pop them
                  // up and poke them
                  double dy = valid.y - current.y;
                  csg::Point3f location = mob->GetLocation() + csg::Point3f(0, dy, 0);
                  LOG(physics.free_motion, 7) << "Popping " << *entity << " up to " << location;
                  mob->MoveTo(location);
               } else {
                  // The entity is floating above a collision shape.  Put them into
                  // free motion so the simulation can take over.
                  LOG(physics.free_motion, 7) << "Putting " << *entity << " into free motion";
                  mob->SetInFreeMotion(true);
                  mob->SetVelocity(csg::Transform(csg::Point3f::zero, csg::Quaternion()));
               }
               break;
            case om::Mob::CLUTTER:
               simulation::Simulation::GetInstance().DestroyEntity(entity->GetObjectId());
               break;
            default:
               NOT_REACHED();
            }
         }
      }
   }
}
