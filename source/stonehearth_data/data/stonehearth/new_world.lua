local MicroWorld = radiant.mods.require('stonehearth_tests.lib.micro_world')
local HeightMapRenderer = radiant.mods.require('stonehearth_terrain.height_map_renderer')
local TerrainGenerator = radiant.mods.require('stonehearth_terrain.terrain_generator')
local Landscaper = radiant.mods.require('stonehearth_terrain.landscaper')
local TerrainType = radiant.mods.require('stonehearth_terrain.terrain_type')

local NewWorld = class(MicroWorld)

function NewWorld:__init()
   self[MicroWorld]:__init()
   self._terrain_generator = TerrainGenerator()
   self:create_world()
   self:place_objects()
end

function NewWorld:create_world()
   local height_map

   height_map = self._terrain_generator:generate_zone(TerrainType.Foothills)
   HeightMapRenderer.render_height_map_to_terrain(height_map, self._terrain_generator.terrain_info)

   local landscaper = Landscaper()

   local i
   for i=1, 3 do
      landscaper:place_forest(height_map)
   end
end

function NewWorld:place_objects()
   local camp_x = 128
   local camp_z = 128

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

   --put some default supplies into the stockpile (for now)
   self:place_item('stonehearth_items.fire_pit_proxy', camp_x+8, camp_z-2)
   self:place_item('stonehearth.oak_log', camp_x+8+1, camp_z-2+1)
   self:place_item('stonehearth.oak_log', camp_x+8, camp_z-2+2)



end

return NewWorld

