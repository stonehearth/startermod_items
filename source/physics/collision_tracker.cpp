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
CollisionTracker::CollisionTracker(NavGrid& ng, TrackerType type, om::EntityPtr entity) :
   ng_(ng),
   _type(type),
   entity_(entity),
   entityId_(entity->GetObjectId())
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
 * -- CollisionTracker::GetEntityId
 *
 */
dm::ObjectId CollisionTracker::GetEntityId() const
{
   return entityId_;
}

/* 
 * -- CollisionTracker::Intersects
 *
 * Returns whether or not the specified region intersects with the region
 * tracked by this collision tracker.  The `region` should be specified in
 * world coodrinates.
 *
 */
bool CollisionTracker::Intersects(csg::Region3 const& region) const
{
   return Intersects(region, region.GetBounds());
}


/* 
 * -- CollisionTracker::Intersects
 *
 * Returns whether or not the specified region intersects with the region
 * tracked by this collision tracker.  The `region` should be specified in
 * world coodrinates, and `regionBounds` must be the exact bounding box around
 * the region.  This method is provided publicly for performance purposes.
 * Use it over ::Intersects(csg::Region const&) if you need to call with the
 * same region on many Trackers.
 *
 */
bool CollisionTracker::Intersects(csg::Region3 const& region, csg::Cube3 const& regionBounds) const
{
   if (Intersects(regionBounds)) {
      for (csg::Cube3 const& cube : region) {
         if (Intersects(cube)) {
            return true;
         }
      }
   }
   return false;
}

TrackerType CollisionTracker::GetType() const
{
   return _type;
}
