#include "radiant.h"
#include "nav_grid.h"
#include "collision_tracker.h"
#include "om/entity.h"
#include "om/components/mob.ridl.h"

using namespace radiant;
using namespace radiant::phys;

/* 
 * -- CollisionTracker::CollisionTracker
 *
 * Nothing to see here, move along...
 */
CollisionTracker::CollisionTracker(NavGrid& ng, om::EntityPtr entity) :
   ng_(ng),
   entity_(entity)
{
}

/* 
 * -- CollisionTracker::Initialize
 *
 * Call MarkChanged() if the entity owned by this tracker moves.  Derived classes,
 * be sure to call CollisionTracker::Initialize in your implementation!
 */
void CollisionTracker::Initialize()
{
   om::EntityPtr entity = entity_.lock();
   if (entity) {
      om::MobPtr mob = entity->GetComponent<om::Mob>();
      if (mob) {
         trace_  = mob->TraceTransform("nav grid", GetNavGrid().GetTraceCategory())
            ->OnModified([this]() {
               MarkChanged();
            });
      }
   }
}

/* 
 * -- CollisionTracker::GetNavGrid
 *
 * Get the NavGrid instance managing this tracker.
 */
NavGrid& CollisionTracker::GetNavGrid() const
{
   return ng_;
}

/* 
 * -- CollisionTracker::GetEntity
 *
 * Get the om::Entity associated with this tracker.  May return nullptr
 * if the entity has been deleted (or was never set in the first place, like
 * in the TerrainTileTracker).
 */
om::EntityPtr CollisionTracker::GetEntity() const
{
   return entity_.lock();
}

/* 
 * -- CollisionTracker::GetEntityPosition
 *
 * Get the world position of the entity associated with this tracker.
 */
csg::Point3 CollisionTracker::GetEntityPosition() const
{
   csg::Point3 position(csg::Point3::zero);
   om::EntityPtr entity = entity_.lock();
   if (entity) {
      om::MobPtr mob = entity->GetComponent<om::Mob>();
      if (mob) {
         position = mob->GetWorldGridLocation();
      }
   }
   return position;
}
