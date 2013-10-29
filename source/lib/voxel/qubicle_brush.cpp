#include "pch.h"
#include <bitset>
#include "csg/util.h"
#include "qubicle_file.h"
#include "qubicle_brush.h"

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

QubicleBrush::QubicleBrush(std::istream& in) :
   normal_(0, 0, -1),
   paint_mode_(Color),
   preserve_matrix_origin_(false)
{
   in >> qubicle_file_;
   qubicle_matrix_ = &qubicle_file_.begin()->second;
}

QubicleBrush::QubicleBrush(QubicleMatrix const* m) :
   normal_(0, 0, -1),
   qubicle_matrix_(m),
   preserve_matrix_origin_(false)
{
}

QubicleBrush& QubicleBrush::SetNormal(csg::Point3 const& normal)
{
   normal_ = normal;
   return *this;
}

QubicleBrush& QubicleBrush::SetPaintMode(PaintMode mode)
{
   paint_mode_ = mode;
   return *this;
}

QubicleBrush& QubicleBrush::SetPreserveMatrixOrigin(bool value)
{
   preserve_matrix_origin_ = value;
   return *this;
}

csg::Region3 QubicleBrush::Paint()
{
   return PreparePaintBrush();
}

csg::Region3 QubicleBrush::PaintThroughStencil(csg::Region3 const& stencil)
{   
   csg::Region3 brush = PreparePaintBrush();
   return IterateThroughStencil(brush, stencil);
}

csg::Region3 QubicleBrush::PreparePaintBrush()
{
   csg::Region3 brush = MatrixToRegion3(*qubicle_matrix_);

   // rotate if necessary...
   if (normal_ != csg::Point3(0, 0, -1)) {
      // this is hacky and stupid.  MatrixToRegion3 has access to the normal.
      // it should simply create boxes with the right position!
      brush = csg::Reface(brush, normal_);
   }
   return brush;
}

csg::Region3 QubicleBrush::IterateThroughStencil(csg::Region3 const& brush,
                                                 csg::Region3 const& stencil)
{
   csg::Region3 model;
   csg::Cube3 brush_bounds = brush.GetBounds();
   csg::Cube3 stencil_bounds = stencil.GetBounds();
   csg::Point3 brush_size = brush_bounds.GetSize();
   csg::Point3 stencil_size = brush_bounds.GetSize();
   csg::Point3 const& brush_min = brush_bounds.GetMin();
   csg::Point3 const& stencil_min = stencil_bounds.GetMin();
   csg::Point3 const& brush_max = brush_bounds.GetMax();
   csg::Point3 const& stencil_max = stencil_bounds.GetMax();

   // Figure out the extents of our iterator...
   csg::Point3 min, max;
   for (int i = 0; i < 3; i++) {
      // how far back do we have to go to get the brush to cover the tile...
      int dist_to_min_edge = std::min(stencil_min[i] - brush_min[i], 0);
      min[i] = csg::GetTileOffset(dist_to_min_edge, brush_size[i]);

      // and how far over?
      int dist_to_max_edge = std::max(stencil_max[i] - brush_max[i], 0);
      max[i] = csg::GetTileOffset(dist_to_max_edge, brush_size[i]) + 1;
   }

   // Now iterate and draw the brush
   for (csg::Point3 i : csg::Cube3(min, max)) {
      csg::Point3 offset = i * brush_size;
      csg::Region3 stamped = brush.Translated(i) & stencil;
      model.AddUnique(stamped);
   }
   return model;
}


csg::Region3 QubicleBrush::MatrixToRegion3(QubicleMatrix const& matrix)
{
   const csg::Point3& size = matrix.GetSize();
   const csg::Point3 pos = matrix.GetPosition();

   csg::Region3 result;

   std::vector<bool> processed;
   processed.resize(size.x * size.y * size.z);


#define OFFSET(x_, y_, z_)          ((y_ * size.z * size.x) + (z_ * size.x) + x_)
#define PROCESSED(x, y, z)          processed[OFFSET(x, y, z)]
#define MATCHES(c, m)               ((paint_mode_ == Color) ? ((c) == (m)) : (csg::Color4::FromInteger(c).a > 0))

   // There it is, that's a straw, you see? You watching? And my straw
   // reaches acroooooooss the room, and starts to drink your milkshake...
   // I... drink... your... milkshake!
   csg::Point3 i;
   for (int y = 0; y < size.y; y++) {
      for (int z = 0; z < size.z; z++) {
         int x = 0;
         while (x < size.x) {
            // skip voxels we've already processed
            if (PROCESSED(x, y, z)) {
               x++;
               continue;
            }
            int color = matrix.At(x, y, z);
            csg::Color4 color4 = csg::Color4::FromInteger(color);

            // fully transparent pixels should be skipped
            if (color4.a == 0) {
               PROCESSED(x, y, z) = true;
               x++;
               continue;
            }

            // we've found an unprocessed pixel, non-transparent at x, y, z.
            // get the biggest rect we can.  start off in the xz-plane
            int x_max, z_max, y_max, u, v, w;
            for (x_max = x + 1; x_max < size.x; x_max++) {
               int c = matrix.At(x_max, y, z);
               if (PROCESSED(x_max, y, z) ||  !MATCHES(c, color)) {
                  goto finished_x;
               }
            }

finished_x:
            for (z_max = z + 1; z_max < size.z; z_max++) {
               for (u = x; u < x_max; u++) {
                  int c = matrix.At(u, y, z_max);
                  if (PROCESSED(u, y, z_max) || !MATCHES(c, color)) {
                     goto finished_xz;
                  }
               }
            }

finished_xz:
            // Now go up in the y direction...
            for (y_max = y + 1; y_max < size.y; y_max++) {
               for (w = z; w < z_max; w++) {
                  for (u = x; u < x_max; u++) {
                     int c = matrix.At(u, y_max, w);
                     if (PROCESSED(u, y_max, w) || !MATCHES(c, color)) {
                        goto finished_xyz;
                     }
                  }
               }
            }

finished_xyz:
            // Add a cube of the current color and mark all this stuff as processed
            csg::Point3 cmin = csg::Point3(x, y, z);
            csg::Point3 cmax = csg::Point3(x_max, y_max, z_max);
            if (preserve_matrix_origin_) {
               cmin += pos;
               cmax += pos;
            }
            result.AddUnique(csg::Cube3(cmin, cmax, color));

            for (v = y; v < y_max; v++) {
               for (w = z; w < z_max; w++) {
                  for (u = x; u < x_max; u++) {
                     PROCESSED(u, v, w) = true;
                  }
               }
            }
            x = x_max;
         }
      }
   }
   for (int i = 0; i < size.x * size.y * size.z; i++) {
      ASSERT(processed[i]);
   }
   // strip off the white space...
   csg::Cube3 bounds = result.GetBounds();
   csg::Point3 offset;
   for (int i = 0; i < 3; i++) {
      offset[i] = std::max(bounds.GetMin()[i], 0);
   }
   if (offset != csg::Point3(0, 0, 0)) {
      result.Translate(-offset);
   }
   return result;
}
