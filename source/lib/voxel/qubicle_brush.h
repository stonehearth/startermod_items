#ifndef _RADIANT_VOXEL_VOXEL_BRUSH_H
#define _RADIANT_VOXEL_VOXEL_BRUSH_H

#include "namespace.h"
#include "csg/region.h"
#include "qubicle_file.h"

BEGIN_RADIANT_VOXEL_NAMESPACE

class QubicleBrush
{
public:
   enum PaintMode {
      Color,
      Opaque,
      Outline
   };

   QubicleBrush(std::istream& in);

   QubicleBrush& SetNormal(csg::Point3 const& normal);
   QubicleBrush& SetPaintMode(PaintMode mode);

   csg::Region3 Paint();
   csg::Region3 PaintThroughStencil(csg::Region3 const& stencil);

private:
   csg::Region3 PreparePaintBrush();
   csg::Region3 MatrixToRegion3(QubicleMatrix const& matrix);
   csg::Region3 IterateThroughStencil(csg::Region3 const& brush, csg::Region3 const& stencil);

private:
   PaintMode             paint_mode_;
   csg::Point3           normal_;
   QubicleMatrix const*  qubicle_matrix_;
   QubicleFile           qubicle_file_;
};

DECLARE_SHARED_POINTER_TYPES(QubicleBrush);

END_RADIANT_VOXEL_NAMESPACE

#endif //  _RADIANT_VOXEL_VOXEL_BRUSH_H
