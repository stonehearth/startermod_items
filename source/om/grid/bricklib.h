#ifndef _RADIANT_OM_GRID_BRICKLIB_H
#define _RADIANT_OM_GRID_BRICKLIB_H

#include "math3d.h"
#include "math3d_collision.h"
#include "om/om.h"

BEGIN_RADIANT_OM_NAMESPACE

class Grid;
class GridTile;

class BrickLib {
   public:
      enum FaceId {
         FACE_NEG_Z = 0,
         FACE_POS_X = 1,
         FACE_POS_Z = 2,
         FACE_NEG_X = 3,
         FACE_POS_Y = 4,
         FACE_NEG_Y = 5,};

      enum MagicVoxels {
         FIRST_DETAIL_VOXEL = 200,
         THREE_BROWN_LOGS = 255,
      };

      struct VertexInfo {
         math3d::point3         position;
         math3d::point3         normal;
         FaceId                 face;

         VertexInfo();
         VertexInfo(math3d::point3 pos, math3d::point3 norm, FaceId face);
      };

      struct FaceInfo {
         std::vector<VertexInfo>     vertices;
         std::vector<int>            indices;
      };


      static math3d::ipoint3 ipoint_0;
      static math3d::ipoint3 ipoint_1;
      static math3d::ipoint3 ipoint_2;
      static math3d::ipoint3 ipoint_3;
      static math3d::ipoint3 ipoint_4;
      static math3d::ipoint3 ipoint_5;
      static math3d::ipoint3 ipoint_6;
      static math3d::ipoint3 ipoint_7;

      static math3d::point3 point_0;
      static math3d::point3 point_1;
      static math3d::point3 point_2;
      static math3d::point3 point_3;
      static math3d::point3 point_4;
      static math3d::point3 point_5;
      static math3d::point3 point_6;
      static math3d::point3 point_7;

      static math3d::point3 normal_neg_x;
      static math3d::point3 normal_neg_y;
      static math3d::point3 normal_neg_z;
      static math3d::point3 normal_pos_x;
      static math3d::point3 normal_pos_y;
      static math3d::point3 normal_pos_z;

      static VertexInfo point_0_neg_x;
      static VertexInfo point_0_neg_y;
      static VertexInfo point_0_neg_z;

      static VertexInfo point_1_pos_x;
      static VertexInfo point_1_neg_y;
      static VertexInfo point_1_neg_z;

      static VertexInfo point_2_pos_x;
      static VertexInfo point_2_neg_y;
      static VertexInfo point_2_pos_z;

      static VertexInfo point_3_neg_x;
      static VertexInfo point_3_neg_y;
      static VertexInfo point_3_pos_z;

      static VertexInfo point_4_neg_x;
      static VertexInfo point_4_pos_y;
      static VertexInfo point_4_neg_z;

      static VertexInfo point_5_pos_x;
      static VertexInfo point_5_pos_y;
      static VertexInfo point_5_neg_z;

      static VertexInfo point_6_pos_x;
      static VertexInfo point_6_pos_y;
      static VertexInfo point_6_pos_z;

      static VertexInfo point_7_neg_x;
      static VertexInfo point_7_pos_y;
      static VertexInfo point_7_pos_z;

      static const math3d::ipoint3 &getFaceNormal(FaceId face);
      static const math3d::ipoint3 &getVertexCoord(FaceId face, int vertex);
      static FaceId getFacing(FaceId direction);

      static math3d::ipoint3 getNeighbor(const math3d::ipoint3 &pt, FaceId face);

      static bool isOcculder(uchar voxel);
      static bool isMagicBrick(uchar voxel);
      static bool isPassable(uchar voxel);

      static const unsigned short *getFaceIndices(const math3d::ipoint3 &offset);

      // xxx: make these batch exporters from the iterator?
      static void addQuad(FaceInfo &fi, FaceId face);
      static void addQuad(FaceInfo &fi, BrickLib::VertexInfo vertices[4]);
};

END_RADIANT_OM_NAMESPACE

#endif // _RADIANT_OM_GRID_BRICK_H
