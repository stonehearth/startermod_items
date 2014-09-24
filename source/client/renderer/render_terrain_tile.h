#ifndef _RADIANT_CLIENT_RENDER_TERRAIN_TILE_H
#define _RADIANT_CLIENT_RENDER_TERRAIN_TILE_H

#include <memory>
#include "namespace.h"
#include "om/region.h"
#include "dm/dm.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class RenderTerrainTile : public std::enable_shared_from_this<RenderTerrainTile>
{
public:
   RenderTerrainTile(RenderTerrain& terrain, csg::Point3 const& location, om::Region3BoxedRef region);
   void UpdateRenderRegion();

private:
   RenderTerrain&          _terrain;
   RenderNodePtr           _node;
   csg::Point3             _location;
   om::Region3BoxedRef     _region;
   csg::Region3            _renderRegion;
   dm::TracePtr            _trace;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_TERRAIN_TILE_H
