#ifndef _RADIANT_PHYSICS_WATER_TIGHT_REGION_H
#define _RADIANT_PHYSICS_WATER_TIGHT_REGION_H

#include <unordered_set>
#include "namespace.h"
#include "core/guard.h"
#include "csg/region.h"
#include "om/tiled_region.h"

BEGIN_RADIANT_PHYSICS_NAMESPACE

/*
 * -- WaterTightRegionBuilder
 *
 * Keeps track of what parts of the world will block water.  This
 * region can be accessed from Lua via the Terrain component's/
 * trace_water_tight_region_delta() and get_water_tight_region() methods
 *
 */

class WaterTightRegionBuilder
{
public: // public methods
   WaterTightRegionBuilder(NavGrid const& ng);
   ~WaterTightRegionBuilder();

   void SetWaterTightRegion(om::Region3TiledPtr region,
                            dm::Boxed<csg::Region3f>* delta);
   void UpdateRegion();
   void ShowDebugShapes(csg::Point3 const& pt, protocol::shapelist* msg);

private: // private methods
   void OnTileDirty(csg::Point3 const& index);
   csg::Region3 GetTileDelta(csg::Point3 const& index);

private: // private types
   typedef std::unordered_set<csg::Point3, csg::Point3::Hash> DirtyTileSet;

private: // instance variables
   NavGrid const&          _navgrid;
   core::Guard             _onTileDirtyGuard;
   DirtyTileSet            _dirtyTiles;

   om::Region3TiledPtr        _tiles;
   dm::Boxed<csg::Region3f>*  _deltaAccumulator;
};

END_RADIANT_PHYSICS_NAMESPACE

#endif //  _RADIANT_PHYSICS_WATER_TIGHT_REGION_H
