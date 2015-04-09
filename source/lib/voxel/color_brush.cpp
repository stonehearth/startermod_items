#include "pch.h"
#include "color_brush.h"

using namespace ::radiant;
using namespace ::radiant::voxel;

std::ostream& operator<<(std::ostream &os, ColorBrush const& brush)
{
   return (os << "[ColorBrush " << brush.GetColor() << "]");
}

ColorBrush::ColorBrush(csg::Color3 const& color) :
   _color(color)
{
}

csg::Region3 ColorBrush::PaintOnce()
{
   csg::Region3 one(csg::Cube3::one);
   one.SetTag(_color.ToInteger());
   return one;
}

csg::Region3 ColorBrush::PaintThroughStencil(csg::Region3 const& stencil)
{
   csg::Region3 result = stencil;
   result.SetTag(_color.ToInteger());
   return result;
}

