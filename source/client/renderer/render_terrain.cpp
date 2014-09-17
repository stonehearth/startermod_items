#include "pch.h"
#include "pipeline.h"
#include "renderer.h"
#include "render_terrain.h"
#include "dm/map_trace.h"
#include "om/components/terrain.ridl.h"
#include "csg/meshtools.h"
#include "lib/perfmon/perfmon.h"
#include "Horde3D.h"
#include <unordered_map>

using namespace ::radiant;
using namespace ::radiant::client;

#define T_LOG(level)      LOG(renderer.terrain, level)

static std::unordered_map<int, csg::Point4f> colorMap_;

static int hex_to_decimal(char c)
{
   if (c >= 'A' && c <= 'F') {
      return 10 + c - 'A';
   }
   if (c >= 'a' && c <= 'f') {
      return 10 + c - 'a';
   }
   if (c >= '0' && c <= '9') {
      return c - '0';
   }
   return 0;
}

static csg::Point4f parse_color(std::string const& str)
{
   csg::Point4f result(0, 0, 0, 1.0f);
   if (str.size() == 7) {
      for (int i = 0; i < 3; i++) {
         result[i] = (float)(hex_to_decimal(str[i*2+1]) * 16 + hex_to_decimal(str[i*2+2])) / 255.0f;
      }
   }
   return result;
}

RenderTerrain::RenderTerrain(const RenderEntity& entity, om::TerrainPtr terrain) :
   entity_(entity),
   terrain_(terrain)
{  
   terrain_root_node_ = H3DNodeUnique(h3dAddGroupNode(entity_.GetNode(), "terrain root node"));
   selected_guard_ = Renderer::GetInstance().SetSelectionForNode(terrain_root_node_.get(), entity_.GetEntity());

   if (colorMap_.empty()) {
      InitalizeColorMap();
   }

   auto on_add_tile = [this](csg::Point3f key, om::Region3fBoxedPtr const& region) {
      csg::Point3 location  = csg::ToClosestInt(key);
      RenderTilePtr render_tile;
      if (region) {
         auto i = tiles_.find(location);
         if (i != tiles_.end()) {
            render_tile = i->second;
         } else {
            render_tile = std::make_shared<RenderTile>();
            render_tile->location = location;
            render_tile->region = region;
            tiles_[location] = render_tile;
         }
         RenderTileRef rt = render_tile;
         render_tile->trace = region->TraceChanges("render", dm::RENDER_TRACES)
                                     ->OnModified([this, rt]{
                                        AddDirtyTile(rt);
                                     })
                                     ->PushObjectState();
      } else {
         tiles_.erase(location);
      }
   };

   auto on_remove_tile = 

   tiles_trace_ = terrain->TraceTiles("render", dm::RENDER_TRACES)
                              ->OnAdded(on_add_tile)
                              ->OnRemoved([this](csg::Point3f const& location) {
                                 tiles_.erase(csg::ToClosestInt(location));
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
   colorMap_[om::Terrain::Bedrock]    = parse_color(config.get("bedrock", unknownColor));
   colorMap_[om::Terrain::RockLayer1] = parse_color(config.get("rock.layer_1", unknownColor));
   colorMap_[om::Terrain::RockLayer2] = parse_color(config.get("rock.layer_2", unknownColor));
   colorMap_[om::Terrain::RockLayer3] = parse_color(config.get("rock.layer_3", unknownColor));
   colorMap_[om::Terrain::RockLayer4] = parse_color(config.get("rock.layer_4", unknownColor));
   colorMap_[om::Terrain::RockLayer5] = parse_color(config.get("rock.layer_5", unknownColor));
   colorMap_[om::Terrain::RockLayer6] = parse_color(config.get("rock.layer_6", unknownColor));
   colorMap_[om::Terrain::SoilLight]  = parse_color(config.get("soil.light", unknownColor));
   colorMap_[om::Terrain::SoilDark]   = parse_color(config.get("soil.dark", unknownColor));
   colorMap_[om::Terrain::GrassEdge1] = parse_color(config.get("grass.edge1", unknownColor));
   colorMap_[om::Terrain::GrassEdge2] = parse_color(config.get("grass.edge2", unknownColor));
   colorMap_[om::Terrain::Grass]      = parse_color(config.get("grass.inner", unknownColor));
   colorMap_[om::Terrain::DirtEdge1]  = parse_color(config.get("dirt.edge1", unknownColor));
   colorMap_[om::Terrain::Dirt]       = parse_color(config.get("dirt.inner", unknownColor));
}

void RenderTerrain::AddDirtyTile(RenderTileRef tile)
{
   dirty_tiles_.push_back(tile);
   
   if (renderer_frame_trace_.Empty()) {
      renderer_frame_trace_ = Renderer::GetInstance().OnRenderFrameStart([=](FrameStartInfo const&) {
         Update();
      });
      ASSERT(!renderer_frame_trace_.Empty());
   }
}

void RenderTerrain::UpdateRenderRegion(RenderTilePtr render_tile)
{
   om::Region3fBoxedPtr region_ptr = render_tile->region.lock();

   if (region_ptr) {
      ASSERT(render_tile);
      csg::Region3 region = csg::ToInt(region_ptr->Get());
      csg::mesh_tools::mesh mesh;
      mesh = csg::mesh_tools().SetColorMap(colorMap_)
                              .ConvertRegionToMesh(region);
   
      RenderNodePtr node = RenderNode::CreateCsgMeshNode(terrain_root_node_.get(), mesh)
                              ->SetMaterial("materials/terrain.material.xml")
                              ->SetUserFlags(UserFlags::Terrain)
                              ->SetTransform(csg::ToFloat(render_tile->location), csg::Point3f::zero, csg::Point3f::one);

      render_tile->SetNode(node);
   }
}

void RenderTerrain::Update()
{
   for (RenderTileRef t : dirty_tiles_) {
      RenderTilePtr tile = t.lock();
      if (tile) {
         UpdateRenderRegion(tile);
      }
   }
   dirty_tiles_.clear();
   renderer_frame_trace_.Clear();
}
