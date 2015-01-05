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
   local player_id = radiant.entities.get_player_id(worker)

   --local bed = self:place_item('stonehearth:furniture:comfy_bed', 0, 0)
   self:place_item_cluster('stonehearth:resources:wood:oak_log', -10, 0, 3, 3)
   local bed = self:place_item('stonehearth:decoration:firepit', 0, 0, player_id)

   local calendar = stonehearth.calendar
   calendar:set_time(21, 59, 0)

end

return MoveItemTest

