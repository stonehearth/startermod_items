#include "../pch.h"
#include "open.h"
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
   std::shared_ptr<std::istream> is = res::ResourceManager2::GetInstance().OpenResource(filename);

   return std::make_shared<QubicleBrush>(*is);
}

NineGridBrushPtr Voxel_CreateNineGridBrush(std::string const& filename)
{
   std::shared_ptr<std::istream> is = res::ResourceManager2::GetInstance().OpenResource(filename);

   return std::make_shared<NineGridBrush>(*is);
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
            lua::RegisterTypePtr<QubicleBrush>()
               .enum_("constants") [
                  value("Color",    QubicleBrush::Color),
                  value("Opaque",   QubicleBrush::Opaque)
               ]
               .def("set_normal",             &QubicleBrush::SetNormal)
               .def("set_paint_mode",         &QubicleBrush::SetPaintMode)
               .def("set_clip_whitespace",    &QubicleBrush::SetClipWhitespace)
               .def("paint_once",             &QubicleBrush::PaintOnce)
               .def("paint_through_stencil",  &QubicleBrush::PaintThroughStencil)
            ,
            lua::RegisterTypePtr<NineGridBrush>()
               .enum_("constants") [
                  value("Color",    NineGridBrush::Color),
                  value("Opaque",   NineGridBrush::Opaque)
               ]
               .def("set_normal",             &NineGridBrush::SetNormal)
               .def("set_paint_mode",         &NineGridBrush::SetPaintMode)
               .def("set_tile_mode",          &NineGridBrush::SetTileMode)
               .def("set_grid_shape",         &NineGridBrush::SetGridShape)
               .def("set_clip_whitespace",    &NineGridBrush::SetClipWhitespace)
               .def("paint_once",             &NineGridBrush::PaintOnce)
               .def("paint_through_stencil",  &NineGridBrush::PaintThroughStencil)
         ]
      ]
   ];
}
