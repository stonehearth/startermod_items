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
   TerrainTileTracker(NavGrid& ng, om::EntityPtr entity, csg::Point3f const& pt, om::Region3fBoxedPtr tile);

   void Initialize() override;
   csg::Region3 GetOverlappingRegion(csg::Cube3 const& bounds) const override;
   bool Intersects(csg::CollisionBox const& worldBounds) const override;

protected:
   void MarkChanged() override;

private:
   NO_COPY_CONSTRUCTOR(TerrainTileTracker)

private:
   om::Region3fBoxedRef region_;
   csg::Point3f         offset_;
   dm::TracePtr         trace_;
   csg::CollisionBox    last_bounds_;
   csg::CollisionShape  shape_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_TERRIN_TRACKER_H
