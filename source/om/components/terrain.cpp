#include "pch.h"
#include "terrain.h"
#include "om/grid/grid.h"
#include "om/entity.h"
#include "mob.h"

using namespace ::radiant;
using namespace ::radiant::om;

luabind::scope Terrain::RegisterLuaType(struct lua_State* L, const char* name)
{
   using namespace luabind;

   return
      class_<Terrain, std::weak_ptr<Component>>(name)
         .def(tostring(self))
         .def("create_new",            &om::Terrain::CreateNew)
         .def("add_region",            &om::Terrain::AddRegion)
         .def("add_cube",              &om::Terrain::AddCube)
         .def("remove_cube",           &om::Terrain::RemoveCube)
         .def("place_entity",          &om::Terrain::PlaceEntity)
         .enum_("TerrainTypes")
         [
            value("BEDROCK",              Terrain::Bedrock),
            value("TOPSOIL",              Terrain::Topsoil),
            value("TOPSOIL_DETAIL",       Terrain::TopsoilDetail),
            value("GRASS",                Terrain::Grass),
            value("PLAINS",               Terrain::Plains),
            value("DARK_WOOD",            Terrain::DarkWood),
            value("DIRT_PATH",            Terrain::DirtPath)
         ]
      ;
}

std::string Terrain::ToString() const
{
   std::ostringstream os;
   os << "(Terrain id:" << GetObjectId() << ")";
   return os.str();
}

void Terrain::CreateNew()
{
}

void Terrain::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("region", region_);
}

void Terrain::AddCube(csg::Cube3 const& cube)
{
   region_.Modify() += cube;
}

void Terrain::RemoveCube(csg::Cube3 const& cube)
{
   region_.Modify() -= cube;
}

void Terrain::AddRegion(csg::Region3 const& region)
{
   region_.Modify() += region;
}

void Terrain::PlaceEntity(EntityRef e, const math3d::ipoint3& pt)
{
   auto entity = e.lock();
   if (entity) {
      int max_y = INT_MIN;
      auto& region = GetRegion();
      for (csg::Cube3 const& cube : region) {
         if (pt.x >= cube.GetMin().x && pt.x < cube.GetMax().x &&
             pt.z >= cube.GetMin().z && pt.z < cube.GetMax().z) {
            max_y = std::max(cube.GetMax().y, max_y);
         }
      }
      if (max_y != INT_MIN) {
         auto mob = entity->GetComponent<Mob>();
         if (mob) {
            mob->MoveToGridAligned(math3d::ipoint3(pt.x, max_y, pt.z));
         }
      }
   }
}
