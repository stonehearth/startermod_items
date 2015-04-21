#ifndef _RADIANT_CLIENT_RENDER_TERRAIN_H
#define _RADIANT_CLIENT_RENDER_TERRAIN_H

#include <map>
#include "namespace.h"
#include "forward_defines.h"
#include "render_component.h"
#include "h3d_resource_types.h"
#include "om/om.h"
#include "om/tiled_region.h"
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
   void MarkAllDirty();
   csg::Point3 GetTileSize();
   void AddCut(om::Region3fBoxedPtr const& cut);
   void RemoveCut(om::Region3fBoxedPtr const& cut);
   void SetClipHeight(int height);
   void EnableXrayMode(bool enabled);
   om::Region3TiledPtr GetXrayTiles();

private:
   void LoadColorMap();
   void ConnectNeighbors(csg::Point3 const& location, RenderTerrainTile& tile, csg::RegionTools3::Plane direction);

   void UpdateNeighbors();
   void UpdateClipPlanes();
   void UpdateGeometry();
   void UpdateLayers();
   void UpdateLayer(RenderTerrainLayer &layer, csg::Point3 const& location);
   void Update();
   void ScheduleUpdate();

   RenderTerrainLayer& GetLayer(csg::Point3 const& location);
   bool LayerIsVisible(csg::Point3 const& location) const;
   void SetLayerVisible(RenderTerrainLayer& layer, bool visible);

   csg::Point3 IndexToLocation(csg::Point3 const& index);
   csg::Point3 LocationToIndex(csg::Point3 const& location);
   csg::Point3 GetNeighborAddress(csg::Point3 const& location, csg::RegionTools3::Plane direction);
   csg::Point3 GetLayerAddressForLocation(csg::Point3 const& location);

   void EachTileIn(csg::Cube3 const& bounds, std::function<void(csg::Point3 const& location, RenderTerrainTile* tile)> cb);
   void EachLayerIn(csg::Cube3 const& bounds, std::function<void(csg::Point3 const& location, RenderTerrainLayer* tile)> cb);

   template <typename T>
   void EachEntryIn(csg::Cube3 const& bounds, T const& map, csg::Point3 const& chunkSize,
                    std::function<void(csg::Point3 const& location, typename T::mapped_type value)> cb);

private:
   typedef std::unordered_set<csg::Point3, csg::Point3::Hash> DirtySet;

private:
   const RenderEntity&  entity_;
   csg::Point3          _tileSize;
   dm::TracePtr         tiles_trace_;
   dm::TracePtr         terrain_config_trace_;
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
   std::unordered_map<dm::ObjectId, csg::Region3> _cutToICut;
   std::unordered_map<dm::ObjectId, dm::TracePtr> _cut_trace_map;
   int                  _clip_height;
   bool                 _enable_xray_mode;
   om::Region3PtrMap    _xray_region_tiles;
   om::Region3TiledPtr  _xray_tiles_accessor;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_TERRAIN_H

