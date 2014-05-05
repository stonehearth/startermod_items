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

static csg::mesh_tools::tesselator_map tess_map;
RenderTerrain::LayerDetailRingInfo RenderTerrain::grass_ring_info_;
RenderTerrain::LayerDetailRingInfo RenderTerrain::dirt_ring_info_;

RenderTerrain::RenderTerrain(const RenderEntity& entity, om::TerrainPtr terrain) :
   entity_(entity),
   terrain_(terrain)
{  
   terrain_root_node_ = H3DNodeUnique(h3dAddGroupNode(entity_.GetNode(), "terrain root node"));
   selected_guard_ = Renderer::GetInstance().TraceSelected(terrain_root_node_.get(), entity_.GetObjectId());

   if (tess_map.empty()) {
      grass_ring_info_.rings.emplace_back(LayerDetailRingInfo::Ring(4, GrassDetailBase));
      grass_ring_info_.rings.emplace_back(LayerDetailRingInfo::Ring(6, GrassDetailBase+1));
      grass_ring_info_.inner = (TerrainDetailTypes)(GrassDetailMax);

      dirt_ring_info_.rings.emplace_back(LayerDetailRingInfo::Ring(8, DirtBase));
      dirt_ring_info_.inner = (TerrainDetailTypes)(DirtMax);

      auto hex_to_decimal = [](char c) -> int {
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
      };

      auto parse_color = [&](std::string const& str) -> csg::Point4f {
         csg::Point4f result(0, 0, 0, 1.0f);
         if (str.size() == 7) {
            for (int i = 0; i < 3; i++) {
               result[i] = (float)(hex_to_decimal(str[i*2+1]) * 16 + hex_to_decimal(str[i*2+2])) / 255.0f;
            }
         }
         return result;
      };

      using boost::property_tree::ptree;
      json::Node config = Renderer::GetInstance().GetTerrainConfig();
      csg::Point4f rock_layer_1_color = parse_color(config.get("rock.layer_1_color", "#ff00ff"));
      csg::Point4f rock_layer_2_color = parse_color(config.get("rock.layer_2_color", "#ff00ff"));
      csg::Point4f rock_layer_3_color = parse_color(config.get("rock.layer_3_color", "#ff00ff"));
      csg::Point4f rock_layer_4_color = parse_color(config.get("rock.layer_4_color", "#ff00ff"));
      csg::Point4f rock_layer_5_color = parse_color(config.get("rock.layer_5_color", "#ff00ff"));
      csg::Point4f rock_layer_6_color = parse_color(config.get("rock.layer_6_color", "#ff00ff"));
      csg::Point4f boulder_color = parse_color(config.get("rock.boulder_color", "#ff00ff"));
      csg::Point4f soil_light_strata_color = parse_color(config.get("soil.light_strata_color", "#ffff00"));
      csg::Point4f soil_dark_strata_color = parse_color(config.get("soil.dark_strata_color", "#ff00ff"));
      csg::Point4f soil_default_color = parse_color(config.get("soil.default_color", "#ff00ff"));
      csg::Point4f soil_detail_color = parse_color(config.get("soil.detail_color", "#ff00ff"));
      csg::Point4f dark_grass_color = parse_color(config.get("grass.dark_color", "#ff00ff"));
      csg::Point4f dark_wood_color = parse_color(config.get("wood.dark_color", "#ff00ff"));

      // xxx: this is in no way thread safe! (see SH-8)
      static csg::Point4f detail_rings[] = {
         parse_color(config.get("grass.ring_0_color", "#ff00ff")),
         parse_color(config.get("grass.ring_1_color", "#ff00ff")),
         parse_color(config.get("grass.ring_2_color", "#ff00ff")),
         parse_color(config.get("dirt.ring_0_color", "#ff00ff")),
         parse_color(config.get("dirt.ring_1_color", "#ff00ff")),
      };

      tess_map[om::Terrain::Soil] = [=](int tag, csg::Point3f const points[], csg::Point3f const& normal, csg::mesh_tools::mesh& m) {
         m.add_face(points, normal, soil_default_color);
      };

      tess_map[om::Terrain::SoilStrata] = [=](int tag, csg::Point3f const points[], csg::Point3f const& normal, csg::mesh_tools::mesh& m) {
         // stripes...
         const int stripe_size = 2;
         if (normal.y) {
            int y = ((int)((points[0].y + stripe_size) / stripe_size)) * stripe_size;
            bool light_stripe = ((y / stripe_size) & 1) != 0;
            m.add_face(points, normal, light_stripe ? soil_light_strata_color : soil_dark_strata_color);
         } else {
            float ymin = std::min(points[0].y, points[1].y);
            float ymax = std::max(points[0].y, points[1].y);

            csg::Point3f stripe[4];
            memcpy(stripe, points, sizeof stripe);

            float y0 = ymin;
            float y1 = (float)((int)((ymin + stripe_size) / stripe_size)) * stripe_size;

            bool light_stripe = (((int)y1 / stripe_size) & 1) != 0;
            while (true) {
               y1 = std::min(y1, ymax);
               stripe[0].y = stripe[3].y = y0;
               stripe[1].y = stripe[2].y = y1;
               m.add_face(stripe, normal, light_stripe ? soil_light_strata_color : soil_dark_strata_color);
               if (y1 == ymax) {
                  break;
               }
               y0 = y1;
               y1 += stripe_size;
               light_stripe = !light_stripe;
            }
         }
      };

      tess_map[om::Terrain::RockLayer1] = [=](int tag, csg::Point3f const points[], csg::Point3f const& normal, csg::mesh_tools::mesh& m) {
         m.add_face(points, normal, rock_layer_1_color);
      };

      tess_map[om::Terrain::RockLayer2] = [=](int tag, csg::Point3f const points[], csg::Point3f const& normal, csg::mesh_tools::mesh& m) {
         m.add_face(points, normal, rock_layer_2_color);
      };

      tess_map[om::Terrain::RockLayer3] = [=](int tag, csg::Point3f const points[], csg::Point3f const& normal, csg::mesh_tools::mesh& m) {
         m.add_face(points, normal, rock_layer_3_color);
      };

      tess_map[om::Terrain::RockLayer4] = [=](int tag, csg::Point3f const points[], csg::Point3f const& normal, csg::mesh_tools::mesh& m) {
         m.add_face(points, normal, rock_layer_4_color);
      };

      tess_map[om::Terrain::RockLayer5] = [=](int tag, csg::Point3f const points[], csg::Point3f const& normal, csg::mesh_tools::mesh& m) {
         m.add_face(points, normal, rock_layer_5_color);
      };

      tess_map[om::Terrain::RockLayer6] = [=](int tag, csg::Point3f const points[], csg::Point3f const& normal, csg::mesh_tools::mesh& m) {
         m.add_face(points, normal, rock_layer_6_color);
      };

      tess_map[om::Terrain::Boulder] = [=](int tag, csg::Point3f const points[], csg::Point3f const& normal, csg::mesh_tools::mesh& m) {
         m.add_face(points, normal, boulder_color);
      };

      tess_map[om::Terrain::DarkGrass] = [=](int tag, csg::Point3f const points[], csg::Point3f const& normal, csg::mesh_tools::mesh& m) {
         m.add_face(points, normal, dark_grass_color);
      };

      tess_map[om::Terrain::Wood] = [=](int tag, csg::Point3f const points[], csg::Point3f const& normal, csg::mesh_tools::mesh& m) {
         m.add_face(points, normal, dark_wood_color);
      };

      auto render_detail = [=](int tag, csg::Point3f const points[], csg::Point3f const& normal, csg::mesh_tools::mesh& m) {
         m.add_face(points, normal, detail_rings[tag - RenderDetailBase]);
      };
      for (int i = RenderDetailBase; i < RenderDetailMax; i++) {
         tess_map[i] = render_detail;
      }
   }
   ASSERT(terrain);

   auto on_add_tile = [this](csg::Point3 location, om::Region3BoxedPtr const& region) {
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

   auto on_remove_tile = [this](csg::Point3 const& location) {
      tiles_.erase(location);
   };

   tiles_trace_ = terrain->TraceTiles("render", dm::RENDER_TRACES)
                              ->OnAdded(on_add_tile)
                              ->OnRemoved([=](csg::Point3 const&) {
                                 NOT_YET_IMPLEMENTED();
                              })
                              ->PushObjectState();
}

RenderTerrain::~RenderTerrain()
{
}

void RenderTerrain::AddDirtyTile(RenderTileRef tile)
{
   dirty_tiles_.push_back(tile);
   
   if (renderer_frame_trace_.Empty()) {
      renderer_frame_trace_ = Renderer::GetInstance().OnRenderFrameStart([=](FrameStartInfo const&) {
         perfmon::TimelineCounterGuard tcg("update terrain");
         Update();
      });
      ASSERT(!renderer_frame_trace_.Empty());
   }
}

void RenderTerrain::UpdateRenderRegion(RenderTilePtr render_tile)
{
   om::Region3BoxedPtr region_ptr = render_tile->region.lock();

   if (region_ptr) {
      ASSERT(render_tile);
      csg::Region3 const& region = region_ptr->Get();
      csg::Region3 tesselatedRegion;

      TesselateTerrain(region, tesselatedRegion);

      csg::mesh_tools::mesh mesh;
      mesh = csg::mesh_tools().SetTesselator(tess_map)
                              .ConvertRegionToMesh(tesselatedRegion);
   
      UniqueRenderable terrainRenderable = Pipeline::GetInstance().AddDynamicMeshNode(terrain_root_node_.get(), mesh, "materials/terrain.material.xml", UserFlags::Terrain);
      h3dSetNodeTransform(terrainRenderable.getNode(), (float)render_tile->location.x, (float)render_tile->location.y, (float)render_tile->location.z, 0, 0, 0, 1, 1, 1);
      render_tile->SetNode(terrainRenderable.getNode());
      // It's okay to just drop UniqueRenderable on the floor, here (at least, right now), since we don't (as yet) free terrain resources.
   }
}

void RenderTerrain::TesselateTerrain(csg::Region3 const& terrain, csg::Region3& tess)
{
   csg::Region3 grass, dirt;

   T_LOG(7) << "Tesselating Terrain...";
   for (csg::Cube3 const& cube : terrain) {
      switch (cube.GetTag()) {
      case om::Terrain::Grass:
         grass.AddUnique(cube);
         break;
      case om::Terrain::Dirt:
         dirt.AddUnique(cube);
         break;
      default:
         tess.AddUnique(cube);
      }
   }

   AddTerrainTypeToTesselation(grass, terrain, tess, grass_ring_info_);
   AddTerrainTypeToTesselation(dirt, csg::Region3(), tess, dirt_ring_info_);
   T_LOG(7) << "Done Tesselating Terrain!";
}

void RenderTerrain::AddTerrainTypeToTesselation(csg::Region3 const& region, csg::Region3 const& terrain, csg::Region3& tess, LayerDetailRingInfo const& ringInfo)
{
   std::unordered_map<int, csg::Region2> layers;

   for (csg::Cube3 const& cube : region) {
      ASSERT(cube.GetMin().y == cube.GetMax().y - 1); // 1 block thin, pizza box
      layers[cube.GetMin().y].AddUnique(csg::Rect2(csg::Point2(cube.GetMin().x, cube.GetMin().z),
                                                   csg::Point2(cube.GetMax().x, cube.GetMax().z)));
   }
   for (auto const& layer : layers) {
      TesselateLayer(layer.second, layer.first, terrain, tess, ringInfo);
   }
}

void RenderTerrain::TesselateLayer(csg::Region2 const& layer, int height, csg::Region3 const& clipper, csg::Region3& tess, LayerDetailRingInfo const& ringInfo)
{
   csg::Region2 inner = layer;
   csg::EdgeListPtr segments = csg::Region2ToEdgeList(layer, height, clipper);

   for (auto const& layer : ringInfo.rings) {
      T_LOG(7) << " Building terrain ring " << height << " " << layer.width;
      csg::Region2 edge = csg::EdgeListToRegion2(segments, layer.width, &inner);      
      for (csg::Rect2 const& rect : edge) {
         csg::Point3 p0(rect.GetMin().x, height, rect.GetMin().y);
         csg::Point3 p1(rect.GetMax().x, height + 1, rect.GetMax().y);
         tess.AddUnique(csg::Cube3(p0, p1, layer.type));
      }
      segments->Inset(layer.width);
      inner -= edge;
   }
   for (csg::Rect2 const& rect : inner) {
      tess.AddUnique(csg::Cube3(csg::Point3(rect.GetMin().x, height, rect.GetMin().y),
                                csg::Point3(rect.GetMax().x, height + 1, rect.GetMax().y),
                                ringInfo.inner));
   }
}

void RenderTerrain::Update()
{
   perfmon::TimelineCounterGuard tcg("tesselate terrain");
   for (RenderTileRef t : dirty_tiles_) {
      RenderTilePtr tile = t.lock();
      if (tile) {
         UpdateRenderRegion(tile);
      }
   }
   dirty_tiles_.clear();
   renderer_frame_trace_.Clear();
}

