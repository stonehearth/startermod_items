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

MeshPtr Mesh_AddVertex(MeshPtr mesh, Vertex const& v)
{
   if (mesh) {
      mesh->vertices.push_back(v);
   }
   return mesh;
}

int Mesh_GetVertexCount(MeshPtr mesh)
{
   if (mesh) {
      return static_cast<int>(mesh->vertices.size());
   }
   return 0;
}

MeshPtr Mesh_AddIndex(MeshPtr mesh, int i)
{
   if (mesh) {
      mesh->indices.push_back(i);
   }
   return mesh;
}

int Mesh_GetIndexCount(MeshPtr mesh)
{
   if (mesh) {
      return static_cast<int>(mesh->indices.size());
   }
   return 0;
}

scope LuaMesh::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterTypePtr_NoTypeInfo<Mesh>("Mesh")
         .def(constructor<>())
         .def("add_vertex",            &Mesh_AddVertex)
         .def("get_vertex_count",      &Mesh_GetVertexCount)
         .def("add_index",             &Mesh_AddIndex)
         .def("get_index_count",       &Mesh_GetIndexCount)
      ,
      lua::RegisterType_NoTypeInfo<Vertex>("Vertex")
         .def(constructor<Point3f const&, float, Point3f const&, Point4f const&>())
         .def(constructor<Point3f const&, float, Point3f const&, Color3 const&>())
         .def(constructor<Point3f const&, float, Point3f const&, Color4 const&>())
         .property("location", &Vertex::GetLocation, &Vertex::SetLocation)
         .property("normal", &Vertex::GetNormal, &Vertex::SetNormal)
         .property("color", &Vertex::GetColor, &Vertex::SetColor)
      ;
}
