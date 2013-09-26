local MicroWorld = require 'lib.micro_world'

local FirepitTest = class(MicroWorld)
--[[
   Instantiate a worker, a stockpile, and a fire pit.
]]

function FirepitTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   --Create the carpenter, bench, and instantiate them to each other

   --local worker = self:place_citizen(12, 12)
   --local faction = worker:get_component('unit_info'):get_faction()

   local tree = self:place_tree(-12, 0)

   --self:place_stockpile_cmd(faction, -10, -10, 2, 2)

   local firepit = self:place_item('stonehearth_items','fire_pit', 0, 0)
   local comfy_bed = self:place_item('stonehearth_items.comfy_bed', 10, 11)
end

return FirepitTest

