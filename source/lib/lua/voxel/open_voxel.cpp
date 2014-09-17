#include "../pch.h"
#include "open.h"
#include "csg/util.h"
#include "resources/res_manager.h"
#include "lib/voxel/qubicle_brush.h"
#include "lib/voxel/nine_grid_brush.h"
#include "lib/json/core_json.h"

using namespace ::radiant;
using namespace ::radiant::voxel;
using namespace luabind;

IMPLEMENT_TRIVIAL_TOSTRING(QubicleBrush)
IMPLEMENT_TRIVIAL_TOSTRING(NineGridBrush)

QubicleBrushPtr Voxel_CreateBrush(std::string const& filename)
{
   voxel::QubicleFile const* q = res::ResourceManager2::GetInstance().LookupQubicleFile(filename);
   if (!q) {
      throw std::logic_error(BUILD_STRING("could not find qubicle file "  << filename));
   }
   return std::make_shared<QubicleBrush>(q);
}

NineGridBrushPtr Voxel_CreateNineGridBrush(std::string const& filename)
{
   voxel::QubicleFile const* q = res::ResourceManager2::GetInstance().LookupQubicleFile(filename);
   if (!q) {
      throw std::logic_error(BUILD_STRING("could not find qubicle file "  << filename));
   }
   return std::make_shared<NineGridBrush>(q);
}

QubicleBrush& QubicleBrush_SetOrigin(QubicleBrush &brush, csg::Point3f const& offset)
{
   brush.SetOrigin(csg::ToClosestInt(offset));
   return brush;
}

QubicleBrush& QubicleBrush_SetNormal(QubicleBrush &brush, csg::Point3f const& normal)
{
   brush.SetNormal(csg::ToClosestInt(normal));
   return brush;
}

csg::Region3f QubicleBrush_PaintOnce(QubicleBrush &brush)
{
   return csg::ToFloat(brush.PaintOnce());
}

csg::Region3f QubicleBrush_PaintThroughStencil(QubicleBrush &brush, csg::Region3f const& stencil)
{
   return csg::ToFloat(brush.PaintThroughStencil(csg::ToInt(stencil)));
}

NineGridBrush &NineGridBrush_SetGridShape(NineGridBrush &brush, csg::Region2f const& shape)
{
   return brush.SetGridShape(csg::ToInt(shape));
}

NineGridBrush &NineGridBrush_SetNormal(NineGridBrush &brush, csg::Point3f const& normal)
{
   return brush.SetNormal(csg::ToInt(normal));
}

csg::Region3f NineGridBrush_PaintThroughStencil(NineGridBrush &brush, csg::Region3f const& stencil)
{
   return csg::ToFloat(brush.PaintThroughStencil(csg::ToInt(stencil)));
}

csg::Region3f NineGridBrush_PaintOnce(NineGridBrush &brush)
{
   return csg::ToFloat(brush.PaintOnce());
}

DEFINE_INVALID_JSON_CONVERSION(QubicleBrush);
DEFINE_INVALID_JSON_CONVERSION(NineGridBrush);

void lua::voxel::open(lua_State* L)
{
   module(L) [
      namespace_("_radiant") [
         namespace_("voxel") [
            def("create_brush",    &Voxel_CreateBrush),
            def("create_nine_grid_brush",    &Voxel_CreateNineGridBrush),
            lua::RegisterTypePtr_NoTypeInfo<QubicleBrush>("QubicleBrush")
               .enum_("constants") [
                  value("Color",    QubicleBrush::Color),
                  value("Opaque",   QubicleBrush::Opaque)
               ]
               .def("set_normal",             &QubicleBrush_SetNormal)
               .def("set_origin",             &QubicleBrush_SetOrigin)
               .def("set_paint_mode",         &QubicleBrush::SetPaintMode)
               .def("set_clip_whitespace",    &QubicleBrush::SetClipWhitespace)
               .def("paint_once",             &QubicleBrush_PaintOnce)
               .def("paint_through_stencil",  &QubicleBrush_PaintThroughStencil)
            ,
            lua::RegisterTypePtr_NoTypeInfo<NineGridBrush>("NineGridBrush")
               .enum_("constants") [
                  value("Color",    NineGridBrush::Color),
                  value("Opaque",   NineGridBrush::Opaque)
               ]
               .enum_("gradiant") [
                  value("Left",     NineGridBrush::NEGATIVE_X),
                  value("Right",    NineGridBrush::POSITIVE_X),
                  value("Back",     NineGridBrush::POSITIVE_Z),
                  value("Front",    NineGridBrush::NEGATIVE_Z)
               ]
               .def("set_normal",             &NineGridBrush_SetNormal)
               .def("set_y_offset",           &NineGridBrush::SetYOffset)
               .def("set_paint_mode",         &NineGridBrush::SetPaintMode)
               .def("set_grid_shape",         &NineGridBrush_SetGridShape)
               .def("set_clip_whitespace",    &NineGridBrush::SetClipWhitespace)
               .def("set_slope",              &NineGridBrush::SetSlope)
               .def("set_max_height",         &NineGridBrush::SetMaxHeight)
               .def("set_gradiant_flags",     &NineGridBrush::SetGradiantFlags)
               .def("paint_once",             &NineGridBrush_PaintOnce)
               .def("paint_through_stencil",  &NineGridBrush_PaintThroughStencil)
         ]
      ]
   ];
}
