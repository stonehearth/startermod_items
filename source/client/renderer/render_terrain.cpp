#include "pch.h"
#include "pipeline.h"
#include "renderer.h"
#include "render_terrain.h"
#include "om/components/terrain.h"
#include "csg/meshtools.h"
#include <unordered_map>

using namespace ::radiant;
using namespace ::radiant::client;

static csg::mesh_tools::tesselator_map tess_map;
RenderTerrain::LayerDetailRingInfo RenderTerrain::foothillGrassRingInfo_;
RenderTerrain::LayerDetailRingInfo RenderTerrain::plainsGrassRingInfo_;
RenderTerrain::LayerDetailRingInfo RenderTerrain::dirtRoadRingInfo_;

RenderTerrain::RenderTerrain(const RenderEntity& entity, om::TerrainPtr terrain) :
   entity_(entity),
   terrain_(terrain)
{
   node_ = h3dAddGroupNode(entity_.GetNode(), "grid");
   h3dSetNodeTransform(node_, -0.5f, 0.0f, -0.5f, 0, 0, 0, 1, 1, 1);

   tracer_ += Renderer::GetInstance().TraceSelected(node_, std::bind(&RenderTerrain::OnSelected, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
   
   if (tess_map.empty()) {
      foothillGrassRingInfo_.rings.emplace_back(LayerDetailRingInfo::Ring(1, GrassDetailBase));
      foothillGrassRingInfo_.rings.emplace_back(LayerDetailRingInfo::Ring(2, GrassDetailBase + 1));
      foothillGrassRingInfo_.rings.emplace_back(LayerDetailRingInfo::Ring(6, GrassDetailBase + 2));
      foothillGrassRingInfo_.rings.emplace_back(LayerDetailRingInfo::Ring(8, GrassDetailBase + 3));
      foothillGrassRingInfo_.inner = (TerrainDetailTypes)(GrassDetailBase + 4);

      plainsGrassRingInfo_.rings.emplace_back(LayerDetailRingInfo::Ring(1,  FoothillsDetailBase));
      plainsGrassRingInfo_.rings.emplace_back(LayerDetailRingInfo::Ring(4,  FoothillsDetailBase + 1));
      plainsGrassRingInfo_.rings.emplace_back(LayerDetailRingInfo::Ring(12, FoothillsDetailBase + 2));
      plainsGrassRingInfo_.inner = (TerrainDetailTypes)(FoothillsDetailBase + 3);

      dirtRoadRingInfo_.rings.emplace_back(LayerDetailRingInfo::Ring(1, DirtRoadBase));
      dirtRoadRingInfo_.inner = (TerrainDetailTypes)(DirtRoadBase + 1);

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
      csg::Point3f topsoil_light = parse_color(config.get<std::string>("topsoil.light_color", "#ffff00"));
      csg::Point3f topsoil_dark = parse_color(config.get<std::string>("topsoil.dark_color", "#ff00ff"));
      csg::Point3f topsoil_detail = parse_color(config.get<std::string>("topsoil.detail_color", "#ff00ff"));
      csg::Point3f plains_color = parse_color(config.get<std::string>("plains.color", "#ff00ff"));
      csg::Point3f dark_wood_color = parse_color(config.get<std::string>("wood.dark_color", "#ff00ff"));
      static csg::Point3f detail_bands[] = {
         parse_color(config.get<std::string>("foothills.band_0_color", "#ff00ff")),
         parse_color(config.get<std::string>("foothills.band_1_color", "#ff00ff")),
         parse_color(config.get<std::string>("foothills.band_2_color", "#ff00ff")),
         parse_color(config.get<std::string>("foothills.band_3_color", "#ff00ff")),
         parse_color(config.get<std::string>("foothills.band_4_color", "#ff00ff")),
         parse_color(config.get<std::string>("plains.band_0_color", "#ff00ff")),
         parse_color(config.get<std::string>("plains.band_1_color", "#ff00ff")),
         parse_color(config.get<std::string>("plains.band_2_color", "#ff00ff")),
         parse_color(config.get<std::string>("plains.band_3_color", "#ff00ff")),
         parse_color(config.get<std::string>("plains.band_4_color", "#ff00ff")),
         parse_color(config.get<std::string>("dirtpath.edges", "#ff00ff")),
         parse_color(config.get<std::string>("dirtpath.center", "#ff00ff")),
      };

      tess_map[om::Terrain::Topsoil] = [=](int tag, csg::Point3f const points[], csg::Point3f const& normal, csg::mesh_tools::mesh& m) {
         // stripes...
         const int stripe_size = 2;
         if (normal.y) {
            int y = ((int)((points[0].y + stripe_size) / stripe_size)) * stripe_size;
            bool light_stripe = ((y / stripe_size) & 1) != 0;
            m.add_face(points, normal, light_stripe ? topsoil_light : topsoil_dark);
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
               m.add_face(stripe, normal, light_stripe ? topsoil_light : topsoil_dark);
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

      tess_map[om::Terrain::TopsoilDetail] = [=](int tag, csg::Point3f const points[], csg::Point3f const& normal, csg::mesh_tools::mesh& m) {
         m.add_face(points, normal, topsoil_detail);
      };

      tess_map[om::Terrain::Plains] = [=](int tag, csg::Point3f const points[], csg::Point3f const& normal, csg::mesh_tools::mesh& m) {
         m.add_face(points, normal, plains_color);
      };

      auto render_detail = [=](int tag, csg::Point3f const points[], csg::Point3f const& normal, csg::mesh_tools::mesh& m) {
         m.add_face(points, normal, detail_bands[tag - RenderDetailBase]);
      };
      for (int i = RenderDetailBase; i < RenderDetailMax; i++) {
         tess_map[i] = render_detail;
      }
   }
   ASSERT(terrain);
   tracer_ += terrain->region_.TraceObjectChanges("terrain renderer", [=](){ UpdateRenderRegion(); });
   UpdateRenderRegion();
}


void RenderTerrain::OnSelected(om::Selection& sel, const math3d::ray3& ray,
                               const math3d::point3& intersection, const math3d::point3& normal)
{
   math3d::ipoint3 brick;

   for (int i = 0; i < 3; i++) {
      // The brick origin is at the center of mass.  Adding 0.5f to the
      // coordinate and flooring it should return a brick coordinate.
      brick[i] = (int)std::floor(intersection[i] + 0.5f);

      // We want to choose the brick that the mouse is currently over.  The
      // intersection point is actually a point on the surface.  So to get the
      // brick, we need to move in the opposite direction of the normal
      if (fabs(normal[i]) > k_epsilon) {
         brick[i] += normal[i] > 0 ? -1 : 1;
      }
   }
   sel.AddBlock(brick);
}

void RenderTerrain::UpdateRenderRegion()
{
   om::TerrainPtr terrain = terrain_.lock();
   if (terrain) {
      csg::Region3 tesselatedRegion;

      TesselateTerrain(terrain->GetRegion(), tesselatedRegion);

      csg::mesh_tools::meshmap meshmap;
      csg::mesh_tools(tess_map).optimize_region(tesselatedRegion, meshmap);
   
      for (auto const& entry : meshmap) {
         H3DNode node = Pipeline::GetInstance().AddMeshNode(node_, entry.second);
      }
   }
}

void RenderTerrain::TesselateTerrain(csg::Region3 const& terrain, csg::Region3& tess)
{
   csg::Region3 grass, plains, dirtroad;

   LOG(WARNING) << "Tesselating Terrain...";
   for (csg::Cube3 const& cube : terrain) {
      switch (cube.GetTag()) {
      case om::Terrain::Grass:
         grass.AddUnique(cube);
         break;
      case om::Terrain::DirtPath:
         dirtroad.AddUnique(cube);
         break;
      case om::Terrain::Plains:
         plains.AddUnique(cube);
         break;
      default:
         tess.AddUnique(cube);
      }
   }

   AddGrassToTesselation(grass,  terrain, tess, foothillGrassRingInfo_);
   AddGrassToTesselation(plains, terrain, tess, plainsGrassRingInfo_);
   AddGrassToTesselation(dirtroad, csg::Region3(), tess, dirtRoadRingInfo_);
   LOG(WARNING) << "Done Tesselating Terrain!";
}

void RenderTerrain::AddGrassToTesselation(csg::Region3 const& grass, csg::Region3 const& terrain, csg::Region3& tess, LayerDetailRingInfo const& ringInfo)
{
   std::unordered_map<int, csg::Region2> grass_layers;

   for (csg::Cube3 const& cube : grass) {
      ASSERT(cube.GetMin().y == cube.GetMax().y - 1); // 1 block thin, pizza box
      grass_layers[cube.GetMin().y].AddUnique(csg::Rect2(csg::Point2(cube.GetMin().x, cube.GetMin().z),
                                                         csg::Point2(cube.GetMax().x, cube.GetMax().z)));
   }
   for (auto const& layer : grass_layers) {
      AddGrassLayerToTesselation(layer.second, layer.first, terrain, tess, ringInfo);
   }
}

void RenderTerrain::AddGrassLayerToTesselation(csg::Region2 const& grass, int height, csg::Region3 const& clipper, csg::Region3& tess, LayerDetailRingInfo const& ringInfo)
{
   // Compute the perimeter of the grass region

   // Add the edge to the tesselation
   csg::Region2 inner = grass;

   csg::EdgeListPtr segments = csg::Region2ToEdgeList(grass, height, clipper);
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

