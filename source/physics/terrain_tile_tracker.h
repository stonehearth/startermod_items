#ifndef _RADIANT_PHYSICS_TERRIN_TILE_TRACKER_H
#define _RADIANT_PHYSICS_TERRIN_TILE_TRACKER_H

#include "collision_tracker.h"
#include "om/om.h"
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
   TerrainTileTracker(NavGrid& ng, om::EntityPtr entity, csg::Point3 const& pt, om::Region3BoxedPtr tile);

   void Initialize() override;
   TrackerType GetType() const override;
   csg::Region3 const& GetLocalRegion() const override;
   csg::Region3 GetOverlappingRegion(csg::Cube3 const& bounds) const override;
   bool Intersects(csg::Cube3 const& worldBounds) const override;

protected:
   void MarkChanged() override;

private:
   NO_COPY_CONSTRUCTOR(TerrainTileTracker)

private:
   om::Region3BoxedRef  region_;
   csg::Point3          offset_;
   dm::TracePtr         trace_;
   csg::Cube3           last_bounds_;
   csg::Region3         localRegion_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_TERRIN_TRACKER_H
