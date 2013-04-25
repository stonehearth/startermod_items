#include "pch.h"
#include "math3d.h"
#include "math3d_collision.h"
#include "bricklib.h"
#include "grid.h"
#include <iomanip>

using namespace ::radiant;
using namespace ::radiant::om;


// see http://paulbourke.net/geometry/polygonise/ diagraom for location of points


math3d::point3 BrickLib::point_0(0, 0, 0);
math3d::point3 BrickLib::point_1(1, 0, 0);
math3d::point3 BrickLib::point_2(1, 0, 1);
math3d::point3 BrickLib::point_3(0, 0, 1);
math3d::point3 BrickLib::point_4(0, 1, 0);
math3d::point3 BrickLib::point_5(1, 1, 0);
math3d::point3 BrickLib::point_6(1, 1, 1);
math3d::point3 BrickLib::point_7(0, 1, 1);

math3d::ipoint3 BrickLib::ipoint_0(0, 0, 0);
math3d::ipoint3 BrickLib::ipoint_1(1, 0, 0);
math3d::ipoint3 BrickLib::ipoint_2(1, 0, 1);
math3d::ipoint3 BrickLib::ipoint_3(0, 0, 1);
math3d::ipoint3 BrickLib::ipoint_4(0, 1, 0);
math3d::ipoint3 BrickLib::ipoint_5(1, 1, 0);
math3d::ipoint3 BrickLib::ipoint_6(1, 1, 1);
math3d::ipoint3 BrickLib::ipoint_7(0, 1, 1);

math3d::point3 BrickLib::normal_neg_x(-1, 0, 0);
math3d::point3 BrickLib::normal_neg_y(0, -1, 0);
math3d::point3 BrickLib::normal_neg_z(0, 0, -1);
math3d::point3 BrickLib::normal_pos_x(1, 0, 0);
math3d::point3 BrickLib::normal_pos_y(0, 1, 0);
math3d::point3 BrickLib::normal_pos_z(0, 0, 1);


BrickLib::VertexInfo BrickLib::point_0_neg_x(BrickLib::point_0, BrickLib::normal_neg_x, FACE_NEG_X);
BrickLib::VertexInfo BrickLib::point_0_neg_y(BrickLib::point_0, BrickLib::normal_neg_y, FACE_NEG_Y);
BrickLib::VertexInfo BrickLib::point_0_neg_z(BrickLib::point_0, BrickLib::normal_neg_z, FACE_NEG_Z);

BrickLib::VertexInfo BrickLib::point_1_pos_x(BrickLib::point_1, BrickLib::normal_pos_x, FACE_POS_X);
BrickLib::VertexInfo BrickLib::point_1_neg_y(BrickLib::point_1, BrickLib::normal_neg_y, FACE_NEG_Y);
BrickLib::VertexInfo BrickLib::point_1_neg_z(BrickLib::point_1, BrickLib::normal_neg_z, FACE_NEG_Z);

BrickLib::VertexInfo BrickLib::point_2_pos_x(BrickLib::point_2, BrickLib::normal_pos_x, FACE_POS_X);
BrickLib::VertexInfo BrickLib::point_2_neg_y(BrickLib::point_2, BrickLib::normal_neg_y, FACE_NEG_Y);
BrickLib::VertexInfo BrickLib::point_2_pos_z(BrickLib::point_2, BrickLib::normal_pos_z, FACE_POS_Z);

BrickLib::VertexInfo BrickLib::point_3_neg_x(BrickLib::point_3, BrickLib::normal_neg_x, FACE_NEG_X);
BrickLib::VertexInfo BrickLib::point_3_neg_y(BrickLib::point_3, BrickLib::normal_neg_y, FACE_NEG_Y);
BrickLib::VertexInfo BrickLib::point_3_pos_z(BrickLib::point_3, BrickLib::normal_pos_z, FACE_POS_Z);

BrickLib::VertexInfo BrickLib::point_4_neg_x(BrickLib::point_4, BrickLib::normal_neg_x, FACE_NEG_X);
BrickLib::VertexInfo BrickLib::point_4_pos_y(BrickLib::point_4, BrickLib::normal_pos_y, FACE_POS_Y);
BrickLib::VertexInfo BrickLib::point_4_neg_z(BrickLib::point_4, BrickLib::normal_neg_z, FACE_NEG_Z);

BrickLib::VertexInfo BrickLib::point_5_pos_x(BrickLib::point_5, BrickLib::normal_pos_x, FACE_POS_X);
BrickLib::VertexInfo BrickLib::point_5_pos_y(BrickLib::point_5, BrickLib::normal_pos_y, FACE_POS_Y);
BrickLib::VertexInfo BrickLib::point_5_neg_z(BrickLib::point_5, BrickLib::normal_neg_z, FACE_NEG_Z);

BrickLib::VertexInfo BrickLib::point_6_pos_x(BrickLib::point_6, BrickLib::normal_pos_x, FACE_POS_X);
BrickLib::VertexInfo BrickLib::point_6_pos_y(BrickLib::point_6, BrickLib::normal_pos_y, FACE_POS_Y);
BrickLib::VertexInfo BrickLib::point_6_pos_z(BrickLib::point_6, BrickLib::normal_pos_z, FACE_POS_Z);

BrickLib::VertexInfo BrickLib::point_7_neg_x(BrickLib::point_7, BrickLib::normal_neg_x, FACE_NEG_X);
BrickLib::VertexInfo BrickLib::point_7_pos_y(BrickLib::point_7, BrickLib::normal_pos_y, FACE_POS_Y);
BrickLib::VertexInfo BrickLib::point_7_pos_z(BrickLib::point_7, BrickLib::normal_pos_z, FACE_POS_Z);

BrickLib::VertexInfo::VertexInfo()
{
}

BrickLib::VertexInfo::VertexInfo(math3d::point3 pos, math3d::point3 norm, FaceId f) :
   position(pos),
   normal(norm),
   face(f)
{
}

const math3d::ipoint3 &BrickLib::getFaceNormal(FaceId face)
{
   static const math3d::ipoint3 faceNormals[] = {
      math3d::ipoint3( normal_neg_z ),    // FACE_NEG_Z
      math3d::ipoint3( normal_pos_x ),    // FACE_POS_X
      math3d::ipoint3( normal_pos_z ),    // FACE_POS_Z
      math3d::ipoint3( normal_neg_x ),    // FACE_NEG_X
      math3d::ipoint3( normal_pos_y ),    // FACE_POS_Y
      math3d::ipoint3( normal_neg_y ),    // FACE_NEG_Y
   };

   return faceNormals[face];
}

const math3d::ipoint3 &BrickLib::getVertexCoord(FaceId face, int vertex)
{
   static math3d::ipoint3 coords[][4] = {
      { ipoint_1, ipoint_0, ipoint_4, ipoint_5, }, // -Z
      { ipoint_1, ipoint_5, ipoint_6, ipoint_2, }, //  X
      { ipoint_6, ipoint_7, ipoint_3, ipoint_2, }, // -X
      { ipoint_3, ipoint_7, ipoint_4, ipoint_0, }, //  Z
      { ipoint_6, ipoint_5, ipoint_4, ipoint_7, }, //  Y
      { ipoint_3, ipoint_0, ipoint_1, ipoint_2, }, // -Y
   };
   return coords[face][vertex];
}



math3d::ipoint3 BrickLib::getNeighbor(const math3d::ipoint3 &pt, FaceId face)
{
   return pt + getFaceNormal(face);
}

BrickLib::FaceId BrickLib::getFacing(BrickLib::FaceId direction)
{
   static const FaceId __faceOppositeSide[] = {
      FACE_POS_Z,    // FACE_NEG_Z
      FACE_NEG_X,    // FACE_POS_X
      FACE_NEG_Z,    // FACE_POS_Z
      FACE_POS_X,    // FACE_NEG_X
      FACE_NEG_Y,    // FACE_POS_Y
      FACE_POS_Y,    // FACE_NEG_Y
   };
   return __faceOppositeSide[direction];
}

bool BrickLib::isOcculder(radiant::uchar voxel)
{
   return voxel > 0 && voxel < FIRST_DETAIL_VOXEL;
}

bool BrickLib::isMagicBrick(radiant::uchar voxel)
{
   return voxel >= FIRST_DETAIL_VOXEL;
}

void BrickLib::addQuad(FaceInfo &fi, FaceId face)
{
   static VertexInfo faces[][4] = {
      { point_1_neg_z, point_0_neg_z, point_4_neg_z, point_5_neg_z, }, // -Z
      { point_1_pos_x, point_5_pos_x, point_6_pos_x, point_2_pos_x, }, //  X
      { point_6_pos_z, point_7_pos_z, point_3_pos_z, point_2_pos_z, }, // -X
      { point_3_neg_x, point_7_neg_x, point_4_neg_x, point_0_neg_x, }, //  Z
      { point_5_pos_y, point_4_pos_y, point_7_pos_y, point_6_pos_y, }, //  Y
      { point_0_neg_y, point_1_neg_y, point_2_neg_y, point_3_neg_y, }, // -Y
   };
   addQuad(fi, faces[face]);
}

const unsigned short *BrickLib::getFaceIndices(const math3d::ipoint3 &offset)
{
   static unsigned short ifaces[2][6] = {
      { 0, 1, 2,    2, 3, 0, },
      { 1, 2, 3,    3, 0, 1, }
   };
   int winding = ((offset.x + offset.y + offset.z) & 1) != 0;
   return ifaces[winding];
}

void BrickLib::addQuad(FaceInfo &fi, BrickLib::VertexInfo vertices[4])
{
   // xxx: need the point to get the right vertex winding
   math3d::ipoint3 pt(0, 0, 0);
   const unsigned short *ifaces = getFaceIndices(pt);

   int ioffset = fi.vertices.size();
   for (int i = 0; i < 6; i++) {
      fi.indices.push_back(ifaces[i] + ioffset);
   }

   for (int i = 0; i < 4; i++) {
      fi.vertices.push_back(vertices[i]);
   }   
}

bool BrickLib::isPassable(radiant::uchar voxel)
{
   return voxel == 0 || voxel >= FIRST_DETAIL_VOXEL;
}
