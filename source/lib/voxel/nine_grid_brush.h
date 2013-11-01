#ifndef _RADIANT_VOXEL_NINE_GRID_BRUSH_H
#define _RADIANT_VOXEL_NINE_GRID_BRUSH_H

#include "qubicle_brush.h"

BEGIN_RADIANT_VOXEL_NAMESPACE

class NineGridBrush
{
public:
   enum PaintMode {
      Color = QubicleBrush::Color,
      Opaque = QubicleBrush::Opaque,
   };
   enum TileMode {
      NineGridLeftToRight,
   };

   NineGridBrush(std::istream& in);

   NineGridBrush& SetNormal(csg::Point3 const& normal);
   NineGridBrush& SetPaintMode(PaintMode mode);
   NineGridBrush& SetTileMode(TileMode mode);
   NineGridBrush& SetGridShape(csg::Region2 const& shape);

   csg::Region3 PaintThroughStencil(csg::Region3 const& stencil);
   csg::Region3 PaintOnce();

private:
   csg::Region3 PreparePaintBrush(QubicleMatrix const*);
   csg::Region3 MatrixToRegion3(QubicleMatrix const& matrix);
   csg::Region3 PaintThroughStencilPtr(csg::Region3 const* stencil);
   csg::Region3 IterateThroughStencil(csg::Region3 const& brush, csg::Region3 const& stencil);
   void PaintThroughStencilClipped(csg::Region3 &dst, int i, int y, csg::Region3 const* stencil, csg::Point2 const& min, csg::Point2 const& max);

private:
   PaintMode             paint_mode_;
   TileMode              tile_mode_;
   csg::Point3           normal_;
   csg::Region2          shape_region2_;
   QubicleFile           qubicle_file_;
   QubicleBrush          nine_grid_[10];
   csg::Point2           brush_sizes_[10];
};

DECLARE_SHARED_POINTER_TYPES(NineGridBrush);

END_RADIANT_VOXEL_NAMESPACE

#endif //  _RADIANT_VOXEL_NINE_GRID_BRUSH_H
