local MicroWorld = require 'lib.micro_world'

local MoveItemTest = class(MicroWorld)
--[[
   Instantiate 2 workers and a bed. Set the time of day to night time, so
   the worker will run to his bed and go to sleep. Instruct worker 2 to 
   move the be and observe what happens. 
]]

function MoveItemTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local worker = self:place_citizen(-8, 8)
   local worker2 = self:place_citizen(-8, 10)
   local faction = radiant.entities.get_faction(worker)

   --local bed = self:place_item('stonehearth:comfy_bed', 0, 0)
   self:place_item_cluster('stonehearth:oak_log', -10, 0, 3, 3)
   local bed = self:place_item('stonehearth:firepit', 0, 0, faction)

   local calendar = radiant.mods.load('stonehearth').calendar
   calendar:set_time(21, 59, 0)

end

return MoveItemTest

