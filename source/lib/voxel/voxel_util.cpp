#if 0
#include "pch.h"
#include <bitset>
#include "csg/region.h"
#include "voxel_util.h"
#include "qubicle_file.h"

using namespace ::radiant;
using namespace ::radiant::voxel;

static const struct {
   int i, u, v, mask;
   csg::Point3f normal;
} qubicle_node_edges__[] = {
   { 2, 0, 1, FRONT_MASK,  csg::Point3f( 0,  0,  1) },
   { 2, 0, 1, BACK_MASK,   csg::Point3f( 0,  0, -1) },
   { 1, 0, 2, TOP_MASK,    csg::Point3f( 0,  1,  0) },
   { 1, 0, 2, BOTTOM_MASK, csg::Point3f( 0, -1,  0) },
   { 0, 2, 1, RIGHT_MASK,  csg::Point3f(-1,  0,  0) },
   { 0, 2, 1, LEFT_MASK,   csg::Point3f( 1,  0,  0) },
};

csg::Region3 voxel::MatrixToRegion3(QubicleMatrix const& matrix)
{
   // Super greedy...   
   const csg::Point3& size = m.GetSize();
   const csg::Point3f matrixPosition((float)m.position_.x, (float)m.position_.y, (float)m.position_.z);

   csg::Region3 result;

   std::bitset<size.x * size.y * size.z> mask;
   mask.reset();

   // There it is, that's a straw, you see? You watching?. And my straw
   // reaches acroooooooss the room, and starts to drink your milkshake...
   // I... drink... your... milkshake!
   csg::Point3 i;
   for (int z = 0; z < size.z; z++) {
      for (int y = 0; y < size.y; y++) {
         int x = 0;
         while (x < size.x) {
            if (mask.test(
         }
      }
   }
   
   for (const auto &edge : qubicle_node_edges__) {
      csg::Point3 pt;
      for (int i = 0; i < size[edge.i]; i++) {
         int maskSize = 0, offset = 0;

         // compute a mask of the current plane
         for (int v = 0; v < size[edge.v]; v++) {
            for (int u = 0; u < size[edge.u]; u++) {
               pt[edge.i] = i;
               pt[edge.u] = u;
               pt[edge.v] = v;

               uint32 c = m.At(pt.x, pt.y, pt.z); 
               uint32 alpha = (c >> 24);
               if ((alpha & edge.mask) != 0) {
                  // make alpha fully opaque so we can use integer comparision.  setting
                  // alpha to 0 makes black (#000000) fully transparent, which isn't what
                  // we want.
                  mask[offset] = c | 0xFF000000;
                  maskSize++;
               } else {
                  mask[offset] = 0;
               }
               offset++;
            }
         }
         ASSERT(offset <= size[edge.u] * size[edge.v]);

         // Suck rectangles out of the mask as greedily as possible
         offset = 0;
         for (int v = 0; v < size[edge.v] && maskSize > 0; v++) {
            for (int u = 0; u < size[edge.u] && maskSize > 0; ) {
               uint32 c = mask[offset];
               if (!c) {
                  u++;
                  offset++;
               } else {
                  int w, h, t;
                  // grab the biggest rectangle we can...
                  for (w = 1; u + w < size[edge.u]; w++) {
                     if (mask[offset + w] != c) {
                        break;
                     }
                  }
                  // grab the biggest height we can...
                  for (h = 1; v + h < size[edge.v]; h++) {
                     for (t = 0; t < w; t++) {
                        if (mask[offset + t + h*size[edge.u]] != c) {
                           break;
                        }
                     }
                     if (t != w) {
                        break;
                     }
                  }
                  // add a quad for the w/h rectangle...
                  csg::Point3f min = edge.normal / 2;
                  min[edge.i] += (float)i;
                  min[edge.u] += (float)u - 0.5f;
                  min[edge.v] += (float)v - 0.5f;

                  // Fudge factor to accout for the different origins between
                  // the .qb file and the skeleton...
                  min.x += 0.5;
                  min.z += 0.5;
                  min.y += 0.5;


                  VoxelGeometryVertex vertex;
                  COPY_VEC(vertex.normal, edge.normal);
                  csg::Color3 col = m.GetColor(c);
                  vertex.color[0] = col.r / 255.0f;
                  vertex.color[1] = col.g / 255.0f;
                  vertex.color[2] = col.b / 255.0f;

                  // UGGGGGGGGGGGGGE
                  int voffset = vertices.size();
                  
                  // 3DS Max uses a z-up, right handed origin....
                  // The voxels in the matrix are stored y-up, left-handed...
                  // Convert the max origin to y-up, right handed...
                  csg::Point3f yUpOrigin(origin.x, origin.z, origin.y); 

                  // Now convert to y-up, left-handed
                  csg::Point3f yUpLHOrigin(yUpOrigin.x, yUpOrigin.y, -yUpOrigin.z);

                  COPY_VEC(vertex.pos, (min + matrixPosition) - yUpLHOrigin);
                  vertices.push_back(vertex);
                  //vertices.back().pos.z *= -1;

                  vertex.pos[edge.v] += h;
                  vertices.push_back(vertex);
                  //vertices.back().pos.z *= -1;

                  vertex.pos[edge.u] += w;
                  vertices.push_back(vertex);
                  //vertices.back().pos.z *= -1;

                  vertex.pos[edge.v] -= h;
                  vertices.push_back(vertex);
                  //vertices.back().pos.z *= -1;

                  // xxx - the whole conversion bit above can be greatly optimized,
                  // but I don't want to touch it now that it's workin'!

                  if (edge.mask == RIGHT_MASK || edge.mask == FRONT_MASK || edge.mask == BOTTOM_MASK) {
                     indices.push_back(voffset + 2);
                     indices.push_back(voffset + 1);
                     indices.push_back(voffset);

                     indices.push_back(voffset + 3);
                     indices.push_back(voffset + 2);
                     indices.push_back(voffset);
                  } else {
                     indices.push_back(voffset);
                     indices.push_back(voffset + 1);
                     indices.push_back(voffset + 2);

                     indices.push_back(voffset);
                     indices.push_back(voffset + 2);
                     indices.push_back(voffset + 3);
                  }

                  // ...zero out the mask
                  for (int k = v; k < v + h; k++) {
                     for (int j = u; j < u + w; j++) {
                        mask[j + size[edge.u] * k ] = 0;
                     }
                  }

                  // ...and move past this rectangle
                  u += w;
                  offset += w;
                  maskSize -= w * h;

                  ASSERT(offset <= size[edge.u] * size[edge.v]);
                  ASSERT(maskSize >= 0);
               }
            }
         }
      }
   }
}
#endif