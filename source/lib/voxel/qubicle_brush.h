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
   };

   enum OffsetMode {
      File,
      Matrix,
   };

   QubicleBrush();
   QubicleBrush(QubicleFile const*);
   QubicleBrush(QubicleMatrix const*);
   QubicleBrush(QubicleMatrix const*, int);

   QubicleBrush& SetOrigin(csg::Point3 const& offset);
   QubicleBrush& SetNormal(csg::Point3 const& normal);
   QubicleBrush& SetOffsetMode(OffsetMode mode);
   QubicleBrush& SetPaintMode(PaintMode mode);
   QubicleBrush& SetClipWhitespace(bool clip);

   csg::Region3 PaintOnce();
   csg::Region3 PaintThroughStencil(csg::Region3 const& stencil);

private:
   csg::Region3 PreparePaintBrush();
   csg::Region3 MatrixToRegion3(QubicleMatrix const& matrix);
   csg::Region3 IterateThroughStencil(csg::Region3 const& brush, csg::Region3 const& stencil);
   QubicleMatrix Lod(const QubicleMatrix& m, int lod_level);

private:
   int                   lod_level_;
   OffsetMode            offset_mode_;
   PaintMode             paint_mode_;
   csg::Point3           normal_;
   csg::Point3           origin_;
   QubicleMatrix const*  qubicle_matrix_;
   bool                  clip_whitespace_;
};

DECLARE_SHARED_POINTER_TYPES(QubicleBrush);

END_RADIANT_VOXEL_NAMESPACE

#endif //  _RADIANT_VOXEL_VOXEL_BRUSH_H
