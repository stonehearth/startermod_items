#include "pch.h"
#include "lua/register.h"
#include "lua_terrain_component.h"
#include "om/components/terrain.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

scope LuaTerrainComponent::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterDerivedObject<Terrain, Component>()
         .def(tostring(self))
         .def("create_new",            &Terrain::CreateNew)
         .def("add_region",            &Terrain::AddRegion)
         .def("place_entity",          &Terrain::PlaceEntity)
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
