local MicroWorld = radiant.mods.require('stonehearth_tests', 'micro_world')
local HeightMapRenderer = radiant.mods.require('stonehearth_terrain', 'height_map_renderer')
local TerrainGenerator = radiant.mods.require('stonehearth_terrain', 'terrain_generator')
local Landscaper = radiant.mods.require('stonehearth_terrain', 'landscaper')
local ZoneType = radiant.mods.require('stonehearth_terrain', 'zone_type')

local NewWorld = class(MicroWorld)

function NewWorld:__init()
   self[MicroWorld]:__init()
   self._terrain_generator = TerrainGenerator()
   self:create_world()
   self:place_objects()
end

function NewWorld:create_world()
   local height_map

   height_map = self._terrain_generator:generate_zone(ZoneType.Foothills)
   HeightMapRenderer.render_height_map_to_terrain(height_map, self._terrain_generator.zone_params)

   local landscaper = Landscaper()
   landscaper:place_forest(height_map)
   landscaper:place_forest(height_map)
end

function NewWorld:place_objects()
   local camp_x = 100
   local camp_z = 100

   local tree = self:place_tree(camp_x-12, camp_z-12)
   --local worker = self:place_citizen(camp_x-8, camp_z-8)
   
   local worker = self:place_citizen(camp_x-3, camp_z-3)
   self:place_citizen(camp_x+0, camp_z-3)
   self:place_citizen(camp_x+3, camp_z-3)
   self:place_citizen(camp_x-3, camp_z+3)
   self:place_citizen(camp_x+0, camp_z+3)
   self:place_citizen(camp_x+3, camp_z+3)
   self:place_citizen(camp_x-3, camp_z+0)
   self:place_citizen(camp_x+3, camp_z+0)

   local faction = worker:get_component('unit_info'):get_faction()
   
   self:place_stockpile_cmd(faction, camp_x+8, camp_z-2, 4, 4)
end

return NewWorld

