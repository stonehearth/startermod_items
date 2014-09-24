#include "pch.h"
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
   terrain_(terrain)
{  
   terrain_root_node_ = H3DNodeUnique(h3dAddGroupNode(entity_.GetNode(), "terrain root node"));
   selected_guard_ = Renderer::GetInstance().SetSelectionForNode(terrain_root_node_.get(), entity_.GetEntity());

   if (_colorMap.empty()) {
      InitalizeColorMap();
   }

   auto on_add_tile = [this](csg::Point3 key, om::Region3BoxedPtr const& region) {
      csg::Point3 location  = csg::ToClosestInt(key);
      RenderTerrainTilePtr render_tile;
      if (region) {
         auto i = tiles_.find(location);
         if (i != tiles_.end()) {
            render_tile = i->second;
         } else {
            render_tile = std::make_shared<RenderTerrainTile>(*this, location, region);
            tiles_[location] = render_tile;
            MarkDirty(render_tile);
         }
      } else {
         tiles_.erase(location);
      }
   };

   auto on_remove_tile = 

   tiles_trace_ = terrain->TraceTiles("render", dm::RENDER_TRACES)
                              ->OnAdded(on_add_tile)
                              ->OnRemoved([this](csg::Point3 const& location) {
                                 tiles_.erase(location);
                              })
                              ->PushObjectState();
}

RenderTerrain::~RenderTerrain()
{
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

void RenderTerrain::MarkDirty(RenderTerrainTileRef tile)
{
   dirty_tiles_.push_back(tile);
   
   if (renderer_frame_trace_.Empty()) {
      renderer_frame_trace_ = Renderer::GetInstance().OnRenderFrameStart([=](FrameStartInfo const&) {
         Update();
      });
      ASSERT(!renderer_frame_trace_.Empty());
   }
}

void RenderTerrain::Update()
{
   for (RenderTerrainTileRef t : dirty_tiles_) {
      RenderTerrainTilePtr tile = t.lock();
      if (tile) {
         tile->UpdateRenderRegion();
      }
   }
   dirty_tiles_.clear();
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

