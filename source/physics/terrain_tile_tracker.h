#ifndef _RADIANT_PHYSICS_TERRIN_TILE_TRACKER_H
#define _RADIANT_PHYSICS_TERRIN_TILE_TRACKER_H

#include "collision_tracker.h"
#include "om/region.h"
#include "csg/point.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE
   
/* 
 * -- TerrainTileTracker
 *
 * A CollisionTracker for tiles of Terarin components of Entities.  See collision_tracker.h
 * for more details.
 */
class TerrainTileTracker : public CollisionTracker
{
public:
   TerrainTileTracker(NavGrid& ng, om::EntityPtr entity, om::Region3BoxedPtr tile);

   void Initialize() override;
   csg::Region3 GetOverlappingRegion(csg::Cube3 const& bounds) const override;
   bool Intersects(csg::CollisionBox const& worldBounds) const override;
   void ClipRegion(csg::CollisionShape& region) const override;

protected:
   void MarkChanged() override;

private:
   NO_COPY_CONSTRUCTOR(TerrainTileTracker)

private:
   om::Region3BoxedRef  region_;
   dm::TracePtr         trace_;
   csg::CollisionBox    last_bounds_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_TERRIN_TRACKER_H
