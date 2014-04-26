#ifndef _RADIANT_PHYSICS_TERRIN_TRACKER_H
#define _RADIANT_PHYSICS_TERRIN_TRACKER_H

#include "collision_tracker.h"
#include "om/om.h"
#include "csg/point.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE

/* 
 * -- TerrainTracker
 *
 * A CollisionTracker for Terarin components of Entities.  This tracker really doesn't
 * to much: the terrain is just a container for terrain tiles, which contain collision
 * regions.  Hence, all this does is manage TerrainTileTrackers on behalf of the
 * NavGrid.
 *
 * See collision_tracker.h for more details.
 */
class TerrainTracker : public CollisionTracker
{
public:
   TerrainTracker(NavGrid& ng, om::EntityPtr entity, om::TerrainPtr terrain);

   void Initialize() override;
   TrackerType GetType() const override;  
   csg::Region3 GetOverlappingRegion(csg::Cube3 const& bounds) const override;
   bool Intersects(csg::Cube3 const& worldBounds) const override;

protected:
   void MarkChanged() override;

private:
   NO_COPY_CONSTRUCTOR(TerrainTracker)

private:
   om::TerrainRef       terrain_;
   csg::Point3          offset_;
   dm::TracePtr         trace_;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif // _RADIANT_PHYSICS_TERRIN_TRACKER_H