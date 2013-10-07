local MicroWorld = radiant.mods.require('stonehearth_tests.lib.micro_world')
local NewWorld = class(MicroWorld) -- UGGGG. No no no (i'll fix it =)  tony

function NewWorld:__init()
   self[MicroWorld]:__init()

   local wgs = radiant.mods.load('stonehearth').world_generation
   local wg = wgs:create_world(true)

   -- this needs to somehow be dependency injected into the generation service.
   self:at(2000, function()
      self:place_camp()
   end)
end

function NewWorld:place_camp()
   local camp_x = 0
   local camp_z = 0

   --local tree = self:place_tree(camp_x-12, camp_z-12)

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
   self:place_item('stonehearth.fire_pit_proxy', camp_x+8, camp_z-2, faction)
   self:place_item('stonehearth.oak_log', camp_x+8+1, camp_z-2+1)
   self:place_item('stonehearth.oak_log', camp_x+8, camp_z-2+2)
end

return NewWorld

