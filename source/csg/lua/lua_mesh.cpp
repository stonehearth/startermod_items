#include "pch.h"
#include "lib/lua/register.h"
#include "lua_mesh.h"
#include "csg/meshtools.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::csg;

IMPLEMENT_TRIVIAL_TOSTRING(Mesh);
IMPLEMENT_TRIVIAL_TOSTRING(Vertex);

DECLARE_SHARED_POINTER_TYPES(Mesh);

int Mesh_AddVertex(MeshPtr mesh, Vertex const& v)
{
   if (mesh) {
      return mesh->AddVertex(v);
   }
   return -1;
}

MeshPtr Mesh_AddIndex(MeshPtr mesh, int i)
{
   if (mesh) {
      mesh->AddIndex(i);
   }
   return mesh;
}

scope LuaMesh::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterTypePtr_NoTypeInfo<Mesh>("Mesh")
         .def(constructor<>())
         .def("add_vertex",            &Mesh_AddVertex)
         .def("add_index",             &Mesh_AddIndex)
      ,
      lua::RegisterType_NoTypeInfo<Vertex>("Vertex")
         .def(constructor<Point3f const&, Point3f const&, Point4f const&>())
         .def(constructor<Point3f const&, Point3f const&, Color3 const&>())
         .def(constructor<Point3f const&, Point3f const&, Color4 const&>())
         .property("location", &Vertex::GetLocation, &Vertex::SetLocation)
         .property("normal", &Vertex::GetNormal, &Vertex::SetNormal)
         .property("color", &Vertex::GetColor, &Vertex::SetColor)
      ;
}
