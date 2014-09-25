#include "pch.h"
#include "radiant_stdutil.h"
#include "pipeline.h"
#include "renderer.h"
#include "render_terrain.h"
#include "render_terrain_tile.h"
#include "dm/map_trace.h"
#include "om/components/terrain.ridl.h"
#include "csg/meshtools.h"
#include "lib/perfmon/perfmon.h"
#include "Horde3D.h"
#include <unordered_map>

using namespace ::radiant;
using namespace ::radiant::client;

#define T_LOG(level)      LOG(renderer.terrain, level)

RenderTerrain::RenderTerrain(const RenderEntity& entity, om::TerrainPtr terrain) :
   entity_(entity),
   terrain_(terrain),
   _tileSize(terrain->GetTileSize())
{  
   terrain_root_node_ = H3DNodeUnique(h3dAddGroupNode(entity_.GetNode(), "terrain root node"));
   selected_guard_ = Renderer::GetInstance().SetSelectionForNode(terrain_root_node_.get(), entity_.GetEntity());

   if (_colorMap.empty()) {
      InitalizeColorMap();
   }

   auto on_add_tile = [this](csg::Point3 index, om::Region3BoxedPtr const& region) {
      csg::Point3 location = index.Scaled(_tileSize);

      ASSERT(region);
      ASSERT(!stdutil::contains(tiles_, location));

      tiles_.insert(std::make_pair(location, new RenderTerrainTile(*this, location, region)));
      _dirtyNeighbors.insert(location);
      MarkDirty(location);
   };

   auto on_remove_tile = 

   tiles_trace_ = terrain->TraceTiles("render", dm::RENDER_TRACES)
                              ->OnAdded(on_add_tile)
                              ->OnRemoved([this](csg::Point3 const& location) {
                                 NOT_YET_IMPLEMENTED();
                              })
                              ->PushObjectState();
}

RenderTerrain::~RenderTerrain()
{
   for (auto const& entry : tiles_) {
      delete entry.second;
   }
}

void RenderTerrain::InitalizeColorMap()
{
   std::string const unknownColor = "#ff00ff";
   json::Node config = Renderer::GetInstance().GetTerrainConfig();
   _colorMap[om::Terrain::Bedrock]    = csg::Color4::FromString(config.get("bedrock", unknownColor));
   _colorMap[om::Terrain::RockLayer1] = csg::Color4::FromString(config.get("rock.layer_1", unknownColor));
   _colorMap[om::Terrain::RockLayer2] = csg::Color4::FromString(config.get("rock.layer_2", unknownColor));
   _colorMap[om::Terrain::RockLayer3] = csg::Color4::FromString(config.get("rock.layer_3", unknownColor));
   _colorMap[om::Terrain::RockLayer4] = csg::Color4::FromString(config.get("rock.layer_4", unknownColor));
   _colorMap[om::Terrain::RockLayer5] = csg::Color4::FromString(config.get("rock.layer_5", unknownColor));
   _colorMap[om::Terrain::RockLayer6] = csg::Color4::FromString(config.get("rock.layer_6", unknownColor));
   _colorMap[om::Terrain::SoilLight]  = csg::Color4::FromString(config.get("soil.light", unknownColor));
   _colorMap[om::Terrain::SoilDark]   = csg::Color4::FromString(config.get("soil.dark", unknownColor));
   _colorMap[om::Terrain::GrassEdge1] = csg::Color4::FromString(config.get("grass.edge1", unknownColor));
   _colorMap[om::Terrain::GrassEdge2] = csg::Color4::FromString(config.get("grass.edge2", unknownColor));
   _colorMap[om::Terrain::Grass]      = csg::Color4::FromString(config.get("grass.inner", unknownColor));
   _colorMap[om::Terrain::DirtEdge1]  = csg::Color4::FromString(config.get("dirt.edge1", unknownColor));
   _colorMap[om::Terrain::Dirt]       = csg::Color4::FromString(config.get("dirt.inner", unknownColor));
}

void RenderTerrain::MarkDirty(csg::Point3 const& location)
{
   _dirtyGeometry.insert(location);
   _dirtyClipPlanes.insert(location);
   
   if (renderer_frame_trace_.Empty()) {
      renderer_frame_trace_ = Renderer::GetInstance().OnRenderFrameStart([=](FrameStartInfo const&) {
         Update();
      });
      ASSERT(!renderer_frame_trace_.Empty());
   }
}

csg::Point3 RenderTerrain::GetNeighborAddress(csg::Point3 const& location, Neighbor direction)
{
   ASSERT(direction >= 0 && direction < NUM_NEIGHBORS);

   switch (direction) {
   case FRONT:
      return location + csg::Point3(0, 0, -_tileSize);
   case BACK:
      return location + csg::Point3(0, 0, _tileSize);
   case LEFT:
      return location + csg::Point3(-_tileSize, 0, 0);
   case RIGHT:
      return location + csg::Point3(_tileSize, 0, 0);
   case TOP:
      return location + csg::Point3(0, _tileSize, 0);
   case BOTTOM:
      return location + csg::Point3(0, -_tileSize, 0);
   }
   NOT_REACHED();
   return csg::Point3::zero;
}

Neighbor RenderTerrain::GetNeighbor(Neighbor direction)
{
   ASSERT(direction >= 0 && direction < NUM_NEIGHBORS);

   switch (direction) {
   case FRONT:
      return BACK;
   case BACK:
      return FRONT;
   case LEFT:
      return RIGHT;
   case RIGHT:
      return LEFT;
   case TOP:
      return BOTTOM;
   case BOTTOM:
      return TOP;
   }
   NOT_REACHED();
   return FRONT;
}

void RenderTerrain::ConnectNeighbors(csg::Point3 const& location, RenderTerrainTile& first, Neighbor direction)
{
   ASSERT(direction >= 0 && direction < NUM_NEIGHBORS);

   auto i = tiles_.find(GetNeighborAddress(location, direction));
   if (i != tiles_.end()) {
      RenderTerrainTile& second = *i->second;
      first.SetClipPlane(direction, second.GetClipPlane(GetNeighbor(direction)));
      second.SetClipPlane(GetNeighbor(direction), first.GetClipPlane(direction));
   }
}

void RenderTerrain::UpdateNeighbors()
{   
   for (csg::Point3 const& location : _dirtyNeighbors) {
      auto i = tiles_.find(location);
      if (i != tiles_.end()) {
         RenderTerrainTile& tile = *i->second;
         for (int d = 0; d < NUM_NEIGHBORS; d++) {
            Neighbor direction = static_cast<Neighbor>(d);
            ConnectNeighbors(location, tile, direction);
         }
      }
   }
   _dirtyNeighbors.clear();
}

void RenderTerrain::UpdateClipPlanes()
{
   for (csg::Point3 const& location : _dirtyClipPlanes) {
      auto i = tiles_.find(location);
      if (i != tiles_.end()) {
         int planesChanged = i->second->UpdateClipPlanes();
         for (int d = 0; d < NUM_NEIGHBORS; d++) {
            Neighbor direction = static_cast<Neighbor>(d);
            if (planesChanged & (1 << d)) {
               _dirtyGeometry.insert(GetNeighborAddress(location, direction));
            }
         }
      }
   }
   _dirtyClipPlanes.clear();
}


void RenderTerrain::UpdateGeometry()
{   
   for (csg::Point3 const& location : _dirtyGeometry) {
      auto i = tiles_.find(location);
      if (i != tiles_.end()) {
         i->second->UpdateGeometry();
      }
   }
   _dirtyGeometry.clear();
}

void RenderTerrain::Update()
{
   UpdateClipPlanes();
   UpdateNeighbors();
   UpdateGeometry();
   renderer_frame_trace_.Clear();
}

csg::TagToColorMap const& RenderTerrain::GetColorMap() const
{
   return _colorMap;
}

H3DNode RenderTerrain::GetGroupNode() const
{
   return terrain_root_node_.get();
}

int RenderTerrain::GetTileSize()
{
   return _tileSize;
}
