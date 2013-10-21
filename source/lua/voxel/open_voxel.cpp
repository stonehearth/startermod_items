#include "../pch.h"
#include "open.h"
#include "resources/res_manager.h"
#include "lib/voxel/qubicle_brush.h"
#include "lib/json/core_json.h"

using namespace ::radiant;
using namespace ::radiant::voxel;
using namespace luabind;

IMPLEMENT_TRIVIAL_TOSTRING(QubicleBrush)

QubicleBrushPtr Voxel_CreateBrush(std::string const& filename)
{
   std::ifstream in;   
   res::ResourceManager2::GetInstance().OpenResource(filename, in);

   return std::make_shared<QubicleBrush>(in);
}

DEFINE_INVALID_JSON_CONVERSION(QubicleBrush);

void lua::voxel::open(lua_State* L)
{
   module(L) [
      namespace_("_radiant") [
         namespace_("voxel") [
            def("create_brush",    &Voxel_CreateBrush),
            lua::RegisterTypePtr<QubicleBrush>()
               .enum_("constants") [
                  value("Color",    QubicleBrush::Color),
                  value("Opaque",   QubicleBrush::Opaque),
                  value("Outline",  QubicleBrush::Outline)
               ]
               .def("set_normal",             &QubicleBrush::SetNormal)
               .def("set_paint_mode",         &QubicleBrush::SetPaintMode)
               .def("paint",                  &QubicleBrush::Paint)
               .def("paint_through_stencil",  &QubicleBrush::PaintThroughStencil)
         ]
      ]
   ];
}
