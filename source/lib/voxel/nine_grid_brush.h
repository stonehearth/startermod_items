#ifndef _RADIANT_VOXEL_NINE_GRID_BRUSH_H
#define _RADIANT_VOXEL_NINE_GRID_BRUSH_H

#include "qubicle_brush.h"
#include "csg/region_tools.h"

BEGIN_RADIANT_VOXEL_NAMESPACE

class NineGridBrush
{
public:
   enum PaintMode {
      Color = QubicleBrush::Color,
      Opaque = QubicleBrush::Opaque,
   };

   enum HeightMode {
      Bounds,
      Region
   };

   enum RiseFlags {
      NEGATIVE_X = (1 << 0),
      POSITIVE_X = (1 << 1),
      NEGATIVE_Z = (1 << 2),
      POSITIVE_Z = (1 << 3),
   };

   NineGridBrush(std::istream& in);

   NineGridBrush& SetNormal(csg::Point3 const& normal);
   NineGridBrush& SetPaintMode(PaintMode mode);
   NineGridBrush& SetGridShape(csg::Region2 const& shape);
   NineGridBrush& SetClipWhitespace(bool clip);
   NineGridBrush& SetSlope(float slope);
   NineGridBrush& SetYOffset(int offset);
   NineGridBrush& SetMaxHeight(int height);
   NineGridBrush& SetGradiantFlags(int flags);

   csg::Region3 PaintOnce();
   csg::Region3 PaintThroughStencil(csg::Region3 const& stencil);

private:
   template <typename T>
   class GridMap
   {
   public:
      GridMap(csg::Rect2 const& bounds, T const& def) :
         _bounds(bounds),
         _m(bounds.GetArea()) 
      {
         std::fill(_m.begin(), _m.end(), def);
      }

      void set(csg::Point2 const& pt, T h) {
         ASSERT(_bounds.Contains(pt));
         csg::Point2 offset = pt - _bounds.min;
         int stride = _bounds.max.x - _bounds.min.x;
         _m[offset.x + (offset.y * stride)] = h;
      };

      T get(csg::Point2 const& pt, T const& def) const {
         if (!_bounds.Contains(pt)) {
            return def;
         }
         csg::Point2 offset = pt - _bounds.min;
         int stride = _bounds.max.x - _bounds.min.x;
         return _m[offset.x + (offset.y * stride)];
      };

   private:
      csg::Rect2 const& _bounds;
      std::vector<T>    _m;
   };

private:
   csg::Region2 ClassifyNineGrid(csg::EdgeMap<int, 2> const& edgeMap);
   csg::Region3 PaintThroughStencilOpt(csg::Region3 const* stencil);
   void ComputeHeightMap(csg::Region2 const& ninegrid, GridMap<short>& heighmap, GridMap<csg::Point2>& gradiant);
   void AddPointToModel(csg::Region3 &model, csg::Point3 const& pt, GridMap<csg::Point2> const& gradiant, csg::Region3 const* modelStencil, int type);

private:
   PaintMode             paint_mode_;
   float                 slope_;
   int                   y_offset_;
   int                   gradiant_;
   int                   maxHeight_;
   csg::Point3           normal_;
   csg::Region2          shape_region2_;
   csg::Rect2            shapeBounds_;
   QubicleFile           qubicle_file_;
   csg::Region3          nine_grid_[10];
   csg::Point2           brush_sizes_[10];
   bool                  clip_whitespace_;
};

DECLARE_SHARED_POINTER_TYPES(NineGridBrush);

END_RADIANT_VOXEL_NAMESPACE

#endif //  _RADIANT_VOXEL_NINE_GRID_BRUSH_H
