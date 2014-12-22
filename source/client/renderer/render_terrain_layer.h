#ifndef _RADIANT_CLIENT_RENDER_TERRAIN_LAYER_H
#define _RADIANT_CLIENT_RENDER_TERRAIN_LAYER_H

#include <memory>
#include "namespace.h"
#include "om/region.h"
#include "csg/region_tools.h"
#include "render_terrain_tile.h"
#include "dm/dm.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class RenderTerrainLayer
{
public:
   RenderTerrainLayer(RenderTerrain& terrain, csg::Point3 const& location);
   ~RenderTerrainLayer();

   RenderNodePtr GetNode();

   void BeginUpdate();
   void AddGeometry(csg::RegionTools3::Plane plane, RenderTerrainTile::Geometry const& g);
   void EndUpdate();

private:
   NO_COPY_CONSTRUCTOR(RenderTerrainLayer);
   
   void AddGeometryToMesh(csg::Mesh& mesh, csg::PlaneInfo3 pi);

private:
   RenderTerrain&                _terrain;
   RenderTerrainTile::Geometry   _geometry[csg::RegionTools3::NUM_PLANES];
   csg::Point3                   _location;
   RenderNodePtr                 _node;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_TERRAIN_LAYER_H
