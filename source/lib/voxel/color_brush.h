#ifndef _RADIANT_VOXEL_COLOR_BRUSH_H
#define _RADIANT_VOXEL_COLOR_BRUSH_H

#include "namespace.h"
#include "csg/region.h"
#include "qubicle_file.h"

BEGIN_RADIANT_VOXEL_NAMESPACE

class ColorBrush
{
public:
   ColorBrush(csg::Color3 const& color);

   csg::Region3 PaintOnce();
   csg::Region3 PaintThroughStencil(csg::Region3 const& stencil);

   csg::Color3 GetColor() const { return _color; }

private:
   csg::Color3       _color;
};

DECLARE_SHARED_POINTER_TYPES(ColorBrush);

END_RADIANT_VOXEL_NAMESPACE

std::ostream& operator<<(std::ostream &o, radiant::voxel::ColorBrush const& brush);

#endif //  _RADIANT_VOXEL_COLOR_BRUSH_H
