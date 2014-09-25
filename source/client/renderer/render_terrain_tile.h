#ifndef _RADIANT_CLIENT_RENDER_TERRAIN_TILE_H
#define _RADIANT_CLIENT_RENDER_TERRAIN_TILE_H

#include <memory>
#include "namespace.h"
#include "om/region.h"
#include "csg/region_tools.h"
#include "dm/dm.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class RenderTerrainTile
{
public:
   RenderTerrainTile(RenderTerrain& terrain, csg::Point3 const& location, om::Region3BoxedRef region);
   int UpdateClipPlanes();
   void UpdateGeometry();

   void SetClipPlane(Neighbor direction, csg::Region2 const* clipPlane);
   csg::Region2 const* GetClipPlane(Neighbor direction);

private:
   csg::Region2 const* GetClipPlaneFor(csg::PlaneInfo3 const& pi);

private:
   RenderTerrain&          _terrain;
   RenderNodePtr           _node;
   csg::Point3             _location;
   om::Region3BoxedRef     _region;
   csg::Region3            _renderRegion;
   csg::Region2            _clipPlanes[NUM_NEIGHBORS];
   csg::Region2 const*     _neighborClipPlanes[NUM_NEIGHBORS];
   dm::TracePtr            _trace;
   csg::RegionTools3       _regionTools;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_TERRAIN_TILE_H
