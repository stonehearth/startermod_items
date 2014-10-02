#ifndef _RADIANT_CLIENT_RENDER_TERRAIN_H
#define _RADIANT_CLIENT_RENDER_TERRAIN_H

#include <map>
#include "namespace.h"
#include "forward_defines.h"
#include "render_component.h"
#include "h3d_resource_types.h"
#include "om/om.h"
#include "dm/dm.h"
#include "csg/util.h"
#include "render_node.h"
#include "render_terrain_tile.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Renderer;

class RenderTerrain : public RenderComponent {
public:
   RenderTerrain(const RenderEntity& entity, om::TerrainPtr terrain);
   ~RenderTerrain();

   csg::TagToColorMap const& GetColorMap() const;
   H3DNode GetGroupNode() const;
   void MarkDirty(csg::Point3 const& location);
   csg::Point3 GetTileSize();

private:
   void InitalizeColorMap();
   void ConnectNeighbors(csg::Point3 const& location, RenderTerrainTile& tile, csg::RegionTools3::Plane direction);

   void UpdateNeighbors();
   void UpdateClipPlanes();
   void UpdateGeometry();
   void UpdateLayers();
   void UpdateLayer(RenderTerrainLayer &layer, csg::Point3 const& location);
   void Update();

   RenderTerrainLayer& GetLayer(csg::Point3 const& location);

   csg::Point3 GetNeighborAddress(csg::Point3 const& location, csg::RegionTools3::Plane direction);
   csg::Point3 GetLayerAddressForLocation(csg::Point3 const& location);

private:
   typedef std::unordered_set<csg::Point3, csg::Point3::Hash> DirtySet;

private:
   const RenderEntity&  entity_;
   csg::Point3          _tileSize;
   dm::TracePtr         tiles_trace_;
   core::Guard          selected_guard_;
   om::TerrainRef       terrain_;
   H3DNodeUnique        terrain_root_node_;
   std::unordered_map<csg::Point3, RenderTerrainTile*, csg::Point3::Hash> tiles_;
   std::unordered_map<csg::Point3, RenderTerrainLayer*, csg::Point3::Hash> layers_;
   DirtySet             _dirtyGeometry;
   DirtySet             _dirtyClipPlanes;
   DirtySet             _dirtyNeighbors;
   DirtySet             _dirtyLayers;
   core::Guard          renderer_frame_trace_;
   csg::TagToColorMap   _colorMap;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_TERRAIN_H
