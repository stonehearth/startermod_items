#ifndef _RADIANT_CLIENT_GEOMETRY_GENERATOR_H
#define _RADIANT_CLIENT_GEOMETRY_GENERATOR_H

#include "om/om.h"
#include "om/grid/bricklib.h" // xxx - no!
#include "om/grid/iterator.h" // xxx - no!
#include "namespace.h"
#include "pipeline.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class GeometryGenerator {
   public:
      GeometryGenerator(const om::GridPtr grid, om::GridTilePtr tile);

   public:
      const Pipeline::Geometry& GetGeometry() const;

      bool Generate();

   protected:
      struct CacheEntry {
         unsigned char     voxel;
         unsigned short    offset;
      };

      void AddFace(const om::BrickIterator &bi, om::BrickLib::FaceId facing);
      int GenerateVertex(int voxel, const math3d::ipoint3 &origin, const math3d::ipoint3 &pt, om::BrickLib::FaceId facing);
      void GenerateVertexNew(int voxel, const math3d::ipoint3 &origin, const math3d::ipoint3 &pt, om::BrickLib::FaceId facing, Vertex &v);
      CacheEntry &GetCacheEntry(int voxel, const math3d::ipoint3 &pt, om::BrickLib::FaceId facing);
      float EstimateAmbientOcculsion(const math3d::ipoint3 &pt, om::BrickLib::FaceId face);

   protected:
      const om::GridPtr             _grid;
      om::GridTilePtr               _tile;
      Pipeline::Geometry            _geo;
      CacheEntry                    _vcache[16+1][16+1][16+1][6][4];
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_PIPELINE_H
