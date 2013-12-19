#ifndef _RADIANT_PHYSICS_NAMESPACE_H
#define _RADIANT_PHYSICS_NAMESPACE_H

#define BEGIN_RADIANT_PHYSICS_NAMESPACE  namespace radiant { namespace phys {
#define END_RADIANT_PHYSICS_NAMESPACE    } }

#define RADIANT_PHYSICS_NAMESPACE    ::radiant::phys

#define IN_RADIANT_PHYSICS_NAMESPACE(x) \
   BEGIN_RADIANT_PHYSICS_NAMESPACE \
   x  \
   END_RADIANT_PHYSICS_NAMESPACE

BEGIN_RADIANT_PHYSICS_NAMESPACE

typedef int TerrainChangeCbId;
typedef std::function<void()> TerrainChangeCb;

class NavGrid;
class CollisionTracker;
class TerrainTracker;
class TerrainTileTracker;
class RegionCollisionShapeTracker;
class VerticalPathingRegionTracker;

DECLARE_SHARED_POINTER_TYPES(CollisionTracker)
DECLARE_SHARED_POINTER_TYPES(TerrainTracker)
DECLARE_SHARED_POINTER_TYPES(TerrainTileTracker)

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_NAMESPACE_H
