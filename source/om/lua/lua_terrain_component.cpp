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
         .def("set_zone_size",         &Terrain::SetZoneSize)
         .def("add_zone",              &Terrain::AddZone)
         .def("place_entity",          &Terrain::PlaceEntity)
         .enum_("BlockTypes")
         [
            value("ROCK_LAYER_1",      Terrain::RockLayer1),
            value("ROCK_LAYER_2",      Terrain::RockLayer2),
            value("ROCK_LAYER_3",      Terrain::RockLayer3),
            value("SOIL",              Terrain::Soil),
            value("DARK_GRASS",        Terrain::DarkGrass),
            value("DARK_GRASS_DARK",   Terrain::DarkGrassDark),
            value("LIGHT_GRASS",       Terrain::LightGrass),
            value("DARK_WOOD",         Terrain::DarkWood),
            value("DIRT_ROAD",         Terrain::DirtRoad)
         ]
      ;
}
