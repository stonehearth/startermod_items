#include "pch.h"
#include "pipeline.h"
#include "renderer.h"
#include "render_terrain.h"
#include "om/components/terrain.h"
#include "csg/meshtools.h"
#include "lib/perfmon/perfmon.h"
#include "Horde3D.h"
#include <unordered_map>

using namespace ::radiant;
using namespace ::radiant::client;

static csg::mesh_tools::tesselator_map tess_map;
RenderTerrain::LayerDetailRingInfo RenderTerrain::light_grass_ring_info_;
RenderTerrain::LayerDetailRingInfo RenderTerrain::dark_grass_ring_info_;
RenderTerrain::LayerDetailRingInfo RenderTerrain::dirt_road_ring_info_;

RenderTerrain::RenderTerrain(const RenderEntity& entity, om::TerrainPtr terrain) :
   entity_(entity),
   terrain_(terrain)
{  
   terrain_root_node_ = H3DNodeUnique(h3dAddGroupNode(entity_.GetNode(), "terrain root node"));
   tracer_ += Renderer::GetInstance().TraceSelected(terrain_root_node_.get(), [this](om::Selection& sel, const csg::Ray3& ray, const csg::Point3f& intersection, const csg::Point3f& normal) {
      OnSelected(sel, ray, intersection, normal);
   });
   tracer_ += Renderer::GetInstance().TraceFrameStart([=]() {
      Update();
   });

   if (tess_map.empty()) {
      light_grass_ring_info_.rings.emplace_back(LayerDetailRingInfo::Ring(4, LightGrassDetailBase));
      light_grass_ring_info_.rings.emplace_back(LayerDetailRingInfo::Ring(6, LightGrassDetailBase+1));
      light_grass_ring_info_.inner = (TerrainDetailTypes)(LightGrassDetailMax);

      dark_grass_ring_info_.rings.emplace_back(LayerDetailRingInfo::Ring(2, DarkGrassDetailBase));
      dark_grass_ring_info_.rings.emplace_back(LayerDetailRingInfo::Ring(3, DarkGrassDetailBase+1));
      dark_grass_ring_info_.inner = (TerrainDetailTypes)(DarkGrassDetailMax);

      dirt_road_ring_info_.rings.emplace_back(LayerDetailRingInfo::Ring(1, DirtRoadBase));
      dirt_road_ring_info_.inner = (TerrainDetailTypes)(DirtRoadMax);

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

      auto parse_color = [&](std::string const& str) -> csg::Point3f {
         csg::Point3f result(0, 0, 0);
         if (str.size() == 7) {
            for (int i = 0; i < 3; i++) {
               result[i] = (float)(hex_to_decimal(str[i*2+1]) * 16 + hex_to_decimal(str[i*2+2])) / 255.0f;
            }
         }
         return result;
      };

      using boost::property_tree::ptree;
      ptree const& config = Renderer::GetInstance().GetConfig();
      csg::Point3f soil_light = parse_color(config.get<std::string>("soil.light_color", "#ffff00"));
      csg::Point3f soil_dark = parse_color(config.get<std::string>("soil.dark_color", "#ff00ff"));
      csg::Point3f soil_detail = parse_color(config.get<std::string>("soil.detail_color", "#ff00ff"));
      csg::Point3f dark_grass_color = parse_color(config.get<std::string>("dark_grass.color", "#ff00ff"));
      csg::Point3f dark_grass_dark_color = parse_color(config.get<std::string>("dark_grass.dark_color", "#ff00ff"));
      csg::Point3f rock_layer_1_color = parse_color(config.get<std::string>("rock.layer_1_color", "#ff00ff"));
      csg::Point3f rock_layer_2_color = parse_color(config.get<std::string>("rock.layer_2_color", "#ff00ff"));
      csg::Point3f rock_layer_3_color = parse_color(config.get<std::string>("rock.layer_3_color", "#ff00ff"));
      csg::Point3f boulder_color = parse_color(config.get<std::string>("rock.boulder_color", "#ff00ff"));
      csg::Point3f dark_wood_color = parse_color(config.get<std::string>("wood.dark_color", "#ff00ff"));

      // xxx: this is in no way thread safe! (see SH-8)
      static csg::Point3f detail_rings[] = {
         parse_color(config.get<std::string>("light_grass.ring_0_color", "#ff00ff")),
         parse_color(config.get<std::string>("light_grass.ring_1_color", "#ff00ff")),
         parse_color(config.get<std::string>("light_grass.ring_2_color", "#ff00ff")),
         parse_color(config.get<std::string>("dark_grass.ring_0_color", "#ff00ff")),
         parse_color(config.get<std::string>("dark_grass.ring_1_color", "#ff00ff")),
         parse_color(config.get<std::string>("dark_grass.ring_2_color", "#ff00ff")),
         parse_color(config.get<std::string>("dirt_road.edges", "#ff00ff")),
         parse_color(config.get<std::string>("dirt_road.center", "#ff00ff")),
      };

      tess_map[om::Terrain::Soil] = [=](int tag, csg::Point3f const points[], csg::Point3f const& normal, csg::mesh_tools::mesh& m) {
         // stripes...
         const int stripe_size = 2;
         if (normal.y) {
            int y = ((int)((points[0].y + stripe_size) / stripe_size)) * stripe_size;
            bool light_stripe = ((y / stripe_size) & 1) != 0;
            m.add_face(points, normal, light_stripe ? soil_light : soil_dark);
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
               m.add_face(stripe, normal, light_stripe ? soil_light : soil_dark);
               if (y1 == ymax) {
                  break;
               }
               y0 = y1;
               y1 += stripe_size;
               light_stripe = !light_stripe;
            }
         }
      };
      tess_map[om::Terrain::DarkWood] = [=](int tag, csg::Point3f const points[], csg::Point3f const& normal, csg::mesh_tools::mesh& m) {
         m.add_face(points, normal, dark_wood_color);
      };

      tess_map[om::Terrain::DarkGrass] = [=](int tag, csg::Point3f const points[], csg::Point3f const& normal, csg::mesh_tools::mesh& m) {
         m.add_face(points, normal, dark_grass_color);
      };

      tess_map[om::Terrain::DarkGrassDark] = [=](int tag, csg::Point3f const points[], csg::Point3f const& normal, csg::mesh_tools::mesh& m) {
         m.add_face(points, normal, dark_grass_dark_color);
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

      tess_map[om::Terrain::Boulder] = [=](int tag, csg::Point3f const points[], csg::Point3f const& normal, csg::mesh_tools::mesh& m) {
         m.add_face(points, normal, boulder_color);
      };

      auto render_detail = [=](int tag, csg::Point3f const points[], csg::Point3f const& normal, csg::mesh_tools::mesh& m) {
         m.add_face(points, normal, detail_rings[tag - RenderDetailBase]);
      };
      for (int i = RenderDetailBase; i < RenderDetailMax; i++) {
         tess_map[i] = render_detail;
      }
   }
   ASSERT(terrain);

   auto on_add_zone = [this](csg::Point3 location, om::Region3BoxedPtr const& region) {
      RenderZonePtr render_zone;
      if (region) {
         auto i = zones_.find(location);
         if (i != zones_.end()) {
            render_zone = i->second;
         } else {
            render_zone = std::make_shared<RenderZone>();
            render_zone->location = location;
            render_zone->region = region;
            zones_[location] = render_zone;
         }
         RenderZoneRef rt = render_zone;
         render_zone->guard = region->TraceObjectChanges("rendering terrain zone", [this, rt]() {
            dirty_zones_.push_back(rt);
         });
         dirty_zones_.push_back(rt);
      } else {
         zones_.erase(location);
      }
   };

   auto on_remove_zone = [this](csg::Point3 const& location) {
      zones_.erase(location);
   };

   auto const& zone_map = terrain->GetZoneMap();
   
   tracer_ += zone_map.TraceMapChanges("terrain renderer", on_add_zone, on_remove_zone);
   for (const auto& entry : zone_map) {
      on_add_zone(entry.first, entry.second);
   }
}

RenderTerrain::~RenderTerrain()
{
}

void RenderTerrain::OnSelected(om::Selection& sel, const csg::Ray3& ray,
                               const csg::Point3f& intersection, const csg::Point3f& normal)
{
   csg::Point3 brick;

   for (int i = 0; i < 3; i++) {
      // The brick origin is at the center of mass.  Adding 0.5f to the
      // coordinate and flooring it should return a brick coordinate.
      brick[i] = (int)std::floor(intersection[i] + 0.5f);

      // We want to choose the brick that the mouse is currently over.  The
      // intersection point is actually a point on the surface.  So to get the
      // brick, we need to move in the opposite direction of the normal
      if (fabs(normal[i]) > csg::k_epsilon) {
         brick[i] += normal[i] > 0 ? -1 : 1;
      }
   }
   sel.AddBlock(brick);
}

void RenderTerrain::UpdateRenderRegion(RenderZonePtr render_zone)
{
   om::Region3BoxedPtr region_ptr = render_zone->region.lock();

   render_zone->Reset();

   if (region_ptr) {
      ASSERT(render_zone);
      csg::Region3 const& region = region_ptr->Get();
      csg::Region3 tesselatedRegion;

      TesselateTerrain(region, tesselatedRegion);

      csg::mesh_tools::mesh mesh;
      mesh = csg::mesh_tools().SetTesselator(tess_map)
                              .ConvertRegionToMesh(tesselatedRegion);
   
      render_zone->node = Pipeline::GetInstance().AddMeshNode(terrain_root_node_.get(), mesh);
      h3dSetNodeTransform(render_zone->node.get(), (float)render_zone->location.x, (float)render_zone->location.y, (float)render_zone->location.z, 0, 0, 0, 1, 1, 1);
   }
}

void RenderTerrain::TesselateTerrain(csg::Region3 const& terrain, csg::Region3& tess)
{
   csg::Region3 light_grass, dark_grass, dirt_road;

   LOG(WARNING) << "Tesselating Terrain...";
   for (csg::Cube3 const& cube : terrain) {
      switch (cube.GetTag()) {
      case om::Terrain::LightGrass:
         light_grass.AddUnique(cube);
         break;
      case om::Terrain::DirtRoad:
         dirt_road.AddUnique(cube);
         break;
      case om::Terrain::DarkGrass:
         dark_grass.AddUnique(cube);
         break;
      default:
         tess.AddUnique(cube);
      }
   }

   AddTerrainTypeToTesselation(light_grass, terrain, tess, light_grass_ring_info_);
   AddTerrainTypeToTesselation(dark_grass, terrain, tess, dark_grass_ring_info_);
   AddTerrainTypeToTesselation(dirt_road, csg::Region3(), tess, dirt_road_ring_info_);
   LOG(WARNING) << "Done Tesselating Terrain!";
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
      LOG(WARNING) << " Building terrain ring " << height << " " << layer.width;
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
   for (RenderZoneRef t : dirty_zones_) {
      RenderZonePtr zone = t.lock();
      if (zone) {
         UpdateRenderRegion(zone);
      }
   }
   dirty_zones_.clear();
}

