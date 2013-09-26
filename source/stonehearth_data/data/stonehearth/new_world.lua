local MicroWorld = radiant.mods.require('stonehearth_tests.lib.micro_world')
local WorldGenerator = radiant.mods.require('stonehearth_terrain.world_generator')

local NewWorld = class(MicroWorld)

function NewWorld:__init()
   self[MicroWorld]:__init()
   local world_generator = WorldGenerator(true)
   world_generator:create_world()

   self:at(2000, function()
      self:place_camp()
   end)
end

function NewWorld:place_camp()
   local camp_x = 0
   local camp_z = 0

   local tree = self:place_tree(camp_x-12, camp_z-12)

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
   self:place_item('stonehearth_trees.oak_log', camp_x+8+1, camp_z-2+1)
   self:place_item('stonehearth_trees.oak_log', camp_x+8, camp_z-2+2)
end

return NewWorld

