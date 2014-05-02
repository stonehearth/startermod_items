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

class OctTree;
class NavGrid;
class NavGridTile;
class NavGridTileData;
class CollisionTracker;
class TerrainTracker;
class TerrainTileTracker;
class RegionCollisionShapeTracker;
class VerticalPathingRegionTracker;
class MobTracker;
class SensorTracker;
class SensorTileTracker;

DECLARE_SHARED_POINTER_TYPES(CollisionTracker)
DECLARE_SHARED_POINTER_TYPES(TerrainTracker)
DECLARE_SHARED_POINTER_TYPES(TerrainTileTracker)
DECLARE_SHARED_POINTER_TYPES(VerticalPathingRegionTracker)
DECLARE_SHARED_POINTER_TYPES(MobTracker)
DECLARE_SHARED_POINTER_TYPES(SensorTracker)
DECLARE_SHARED_POINTER_TYPES(SensorTileTracker)

enum {
   TILE_SIZE = 16
};

/*
 * We keep one 16x16x16 array of bits for every TrackerType, plus a number
 * of derived arrays (e.g. can_stand_).
 * 
 * COLLISION - bits that are on represent voxels which overlap some entities
 *             collision shape.  Mobile entities cannot overlap these voxels.
 *
 * LADDER - represents bits which overlap some entities VerticalPathingRegion
 *          region.  These bits are both traversable (i.e. you can walk through
 *          them) and can support an entity (i.e. you can stand on them).
 *          I guess technically that makes them more like a chute, but \/\/
 *          Ladder bits override collision bits!
 */
enum TrackerType {
   COLLISION = 0,
   LADDER,

   NUM_BIT_VECTOR_TRACKERS,

   MOB,
   MAX_TRACKER_TYPES,
};


END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_NAMESPACE_H
