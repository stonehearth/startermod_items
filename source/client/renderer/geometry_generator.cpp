#include "pch.h"
#include "om/grid/grid.h"
#include "geometry_generator.h"

using ::radiant::math3d::point3;
using ::radiant::math3d::ipoint3;
using ::radiant::math3d::ibounds3;
using ::radiant::client::GeometryGenerator;

using namespace ::radiant;
using namespace ::radiant::client;

GeometryGenerator::GeometryGenerator(const om::GridPtr grid, om::GridTilePtr tile) :
   _grid(grid),
   _tile(tile)
{
   ASSERT(_geo.vertices.size() == 0);
   ASSERT(_geo.indices.size() == 0);
   memset(_vcache, 0, sizeof(_vcache));
   _geo.vertices.reserve(20000);
   _geo.indices.reserve(20000);
}
   
const Pipeline::Geometry& GeometryGenerator::GetGeometry() const
{
   return _geo;
}

bool GeometryGenerator::Generate()
{
   _geo.vertices.clear();
   _geo.indices.clear();

   _tile->iterator().forEachBrick([&](const om::BrickIterator &bi) {
      if (!om::BrickLib::isMagicBrick(bi.brick())) {
         for (int faceId = 0; faceId < 6; faceId++) {
            if (!bi.faceIsObscured((om::BrickLib::FaceId)faceId)) {
               AddFace(bi, (om::BrickLib::FaceId)faceId);
            }
         }
      }
   });
   return _geo.vertices.size() > 0;
}

int GeometryGenerator::GenerateVertex(int voxel, const ipoint3 &origin, const ipoint3 &pt, om::BrickLib::FaceId facing)
{
   if (_geo.vertices.size() == 0) {
      _geo.vertices.resize(1);
   }
   ASSERT(voxel);
   auto &cacheEntry = GetCacheEntry(voxel, pt, facing);
   if (!cacheEntry.offset) {
      cacheEntry.voxel = voxel;
      cacheEntry.offset = _geo.vertices.size();

      _geo.vertices.resize(_geo.vertices.size() + 1);
      GenerateVertexNew(voxel, origin, pt, facing, _geo.vertices.back());
      if (_geo.vertices.size() == 2) {
         _geo.vertices[0] = _geo.vertices[1];
      }
   }
   ASSERT(cacheEntry.offset);
   return cacheEntry.offset;
}

void GeometryGenerator::AddFace(const om::BrickIterator &bi, om::BrickLib::FaceId facing) {
   const ipoint3 &origin = bi.tile().getLocation();
   unsigned short vertices[4];

   for (int i = 0; i < ARRAYSIZE(vertices); i++) {
      vertices[i] = GenerateVertex(bi.brick(), origin, bi.offset() + om::BrickLib::getVertexCoord(facing, i), facing);
   }
   const unsigned short *ifaces = om::BrickLib::getFaceIndices(bi.offset());
   for (int i = 0; i < 6; i++) {
      _geo.indices.push_back(vertices[ifaces[i]]);
   }
}


GeometryGenerator::CacheEntry &GeometryGenerator::GetCacheEntry(int voxel, const ipoint3 &pt, om::BrickLib::FaceId facing) {
   ASSERT(pt.x >= 0 && pt.y >= 0 && pt.z >= 0 &&
          pt.x <= 16 && pt.y <= 16 && pt.z <= 16 &&
          facing >= 0 && facing < 6);

   int count = 0;
   CacheEntry *info = _vcache[pt.y][pt.z][pt.x][facing];
   while (info->offset != 0 && info->voxel != voxel) {
      info++;
      count++;
      ASSERT(count < 4);
   }
   return *info;
}
      
void GeometryGenerator::GenerateVertexNew(int voxel, const ipoint3 &origin, const ipoint3 &pt, om::BrickLib::FaceId facing, Vertex &v) {
   ASSERT(!om::BrickLib::isMagicBrick(voxel));
   static bool showAmbientHistogram = false;
   static bool softwareAmbientOcculsion = false;
   const ipoint3 &normal = om::BrickLib::getFaceNormal(facing);

   v.pos.x = (float)(pt.x + origin.x) - 0.5f;
   v.pos.y = (float)(pt.y + origin.y);
   v.pos.z = (float)(pt.z + origin.z) - 0.5f;
   v.normal.x = (float)normal.x;
   v.normal.y = (float)normal.y;
   v.normal.z = (float)normal.z;

   _grid->getVoxelColor(voxel, v.color);

   if (softwareAmbientOcculsion) {
      float ambient = 1.0f - EstimateAmbientOcculsion(_tile->getLocation() + pt, facing);

      if (!showAmbientHistogram) {
         v.color *= ambient;
      } else {
         v.color = math3d::Histogram::Sample(ambient);
      }
   }
}

// Should make a 3d integral map and compute the occulsion all at once
float GeometryGenerator::EstimateAmbientOcculsion(const ipoint3 &pt, om::BrickLib::FaceId face)
{
   static int queryNormalSize = 2;
   static int queryTangentSize = 3;
   static float occulsionScale = 0.8f;

   ipoint3 seed = pt;
   ibounds3 bounds(seed, seed);

   const ipoint3 &normal = om::BrickLib::getFaceNormal(face);
   for (int i = 0; i < 3; i++) {
      if (normal[i]) { 
         bounds.add_point(seed + (normal * queryNormalSize));
      } else {
         ipoint3 offset = seed;
         offset[i] = seed[i] + queryTangentSize;
         bounds.add_point(offset);
         offset[i] = seed[i] - queryTangentSize;
         bounds.add_point(offset);
      }
   }
   float occlusion = _grid->GetOcculsionEstimate(bounds);

   return std::min(1.0f, occulsionScale * occlusion);
}