#include "pch.h"
#include "nine_grid_brush.h"

using namespace ::radiant;
using namespace ::radiant::voxel;

NineGridBrush::NineGridBrush(std::istream& in) :
   normal_(0, 0, -1),
   tile_mode_(NineGridLeftToRight),
   paint_mode_(Color),
   qubicle_file_("")
{
   in >> qubicle_file_;
}

NineGridBrush& NineGridBrush::SetNormal(csg::Point3 const& normal)
{
   normal_ = normal;
   return *this;
}

NineGridBrush& NineGridBrush::SetPaintMode(PaintMode mode)
{
   paint_mode_ = mode;
   return *this;
}

NineGridBrush& NineGridBrush::SetTileMode(TileMode mode)
{
   tile_mode_ = mode;
   return *this;
}

NineGridBrush& NineGridBrush::SetGridShape(csg::Region2 const& shape)
{
   shape_region2_ = shape;
   return *this;
}

csg::Region3 NineGridBrush::PaintOnce()
{
   return PaintThroughStencilPtr(nullptr);
}

csg::Region3 NineGridBrush::PaintThroughStencil(csg::Region3 const& stencil)
{
   return PaintThroughStencilPtr(&stencil);
}

csg::Region3 NineGridBrush::PaintThroughStencilPtr(csg::Region3 const* stencil)
{   
   csg::Region3 result;

   for (int i = 1; i <= 9; i++) {
      nine_grid_[i] = QubicleBrush(qubicle_file_.GetMatrix(BUILD_STRING(i)))
                          .SetPaintMode((QubicleBrush::PaintMode)paint_mode_)
                          .SetPreserveMatrixOrigin(true)
                          .SetNormal(normal_);

      csg::Point3 bounds = nine_grid_[i].PaintOnce().GetBounds().GetSize();
      brush_sizes_[i] = csg::Point2(bounds.x, bounds.z);
   }
   csg::Point2* bs = brush_sizes_;                 // brush sizes
   csg::Rect2 sb = shape_region2_.GetBounds();     // shape bounds

   csg::Point2 origin_1(sb.min.x, sb.min.y);
   csg::Point2 origin_7(sb.min.x, sb.max.y - bs[7].y);
   csg::Point2 origin_3(sb.max.x - bs[3].x, sb.min.y);
   csg::Point2 origin_9(sb.max.x - bs[9].x, sb.max.y - bs[9].y);

   PaintThroughStencilClipped(result, 1, 0, stencil, origin_1, origin_1 + bs[1]);
   PaintThroughStencilClipped(result, 3, 0, stencil, origin_3, origin_3 + bs[3]);
   PaintThroughStencilClipped(result, 7, 0, stencil, origin_7, origin_7 + bs[7]);
   PaintThroughStencilClipped(result, 9, 0, stencil, origin_9, origin_9 + bs[9]);

   // xxx: works with the current roof, but not quite right...
   csg::Point2 cs = bs[1];

   auto paint_flat_mode = [&]() {
      int y = 0;

      PaintThroughStencilClipped(result, 2, y, stencil, origin_1 + csg::Point2(cs.x, 0),
                                                        origin_3 + csg::Point2(0, cs.y));

      PaintThroughStencilClipped(result, 8, y, stencil, origin_7 + csg::Point2(cs.x, 0),
                                                        origin_9 + csg::Point2(0, cs.y));

      PaintThroughStencilClipped(result, 4, y, stencil, origin_1 + csg::Point2(0, cs.y),
                                                        origin_7 + csg::Point2(cs.x, 0));

      PaintThroughStencilClipped(result, 6, y, stencil, origin_3 + csg::Point2(0, cs.y),
                                                        origin_9 + csg::Point2(cs.x, 0));

      PaintThroughStencilClipped(result, 5, y, stencil, origin_1 + csg::Point2(cs.x, cs.y),
                                                        origin_9);
   };

   // left to right mode...
   //  y = 0    7 8 8 8 8 8 8 9
   //
   //  y = 1    4 5 5 5 5 5 5 6
   //
   //  y = 2    4 5 5 5 5 5 5 6
   //
   //  y = 3    4 5 5 5 5 5 5 6
   //
   //  y = 2    4 5 5 5 5 5 5 6
   //
   //  y = 1    4 5 5 5 5 5 5 6
   //
   //  y = 0    1 2 2 2 2 2 2 3
   auto paint_front_to_back_mode = [&]() {
      int y = 0;

      // Draw the bottom and top rows...
      PaintThroughStencilClipped(result, 2, y, stencil, origin_1 + csg::Point2(cs.x, 0),
                                                        origin_3 + csg::Point2(0, cs.y));

      PaintThroughStencilClipped(result, 8, y, stencil, origin_7 + csg::Point2(cs.x, 0),
                                                        origin_9 + csg::Point2(0, cs.y));

      csg::Point2 offset_4 = origin_1 + csg::Point2(0, cs.y);
      csg::Point2 offset_6 = origin_3 + csg::Point2(0, cs.y);

      auto draw_row = [&](int y, int row) {
         PaintThroughStencilClipped(result, 4, y, stencil, offset_4 + csg::Point2(0, row),
                                                     offset_4 + csg::Point2(cs.x, row + 1));
         PaintThroughStencilClipped(result, 5, y, stencil, offset_4 + csg::Point2(cs.x, row),
                                                     offset_6 + csg::Point2(0, row + 1));
         PaintThroughStencilClipped(result, 6, y, stencil, offset_6 + csg::Point2(0, row),
                                                     offset_6 + csg::Point2(cs.x, row + 1));
      };

      // Run through the rest...
      int max_y = sb.GetSize().y - 2; // skip the border on either side..
      int i, c = max_y / 2;

      for (i = 0; i < c; i++) {
         draw_row(i + 1, i);
         draw_row(i + 1, max_y - i - 1);
      }
      if (c * 2 != max_y) {
         draw_row(i + 1, i);
      }
   };

   paint_front_to_back_mode();

   return result;
}

void NineGridBrush::PaintThroughStencilClipped(csg::Region3 &dst, int i, int y, csg::Region3 const* stencil, csg::Point2 const& min, csg::Point2 const& max)
{
   csg::Point3 box_min(min.x, y, min.y);
   csg::Point3 box_max(max.x, y + brush_sizes_[i].y, max.y);
   csg::Cube3 grid_box = csg::Cube3::Construct(box_min, box_max);
   if (!grid_box.IsEmpty()) {
      if (stencil) {
         csg::Region3 clipped = *stencil & grid_box;
         dst.AddUnique(nine_grid_[i].PaintThroughStencil(clipped));
      } else {
         dst.AddUnique(nine_grid_[i].PaintThroughStencil(grid_box));
      }
   }
}
