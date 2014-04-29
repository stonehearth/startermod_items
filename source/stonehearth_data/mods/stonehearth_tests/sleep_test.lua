local MicroWorld = require 'lib.micro_world'

local SleepTest = class(MicroWorld)
--[[
   Instantiate a worker and a bed. Set the time of day to night time, so
   the worker will run to his bed and go to sleep.
   If there is a second worker, check that he sleeps on the floor.
]]

function SleepTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local w1 = self:place_citizen(-8, 8)
   local w2 = self:place_citizen(8, 8)
   self:place_item('stonehearth:comfy_bed', 0, 8)
   --self:place_item('stonehearth:comfy_bed', 0, 0)
   local tree = self:place_tree(-12, -12)

   --Make sure pets sleep too 
   local town = stonehearth.town:get_town(w1)
   local critter = self:place_item('stonehearth:red_fox', 2, 2)
   local equipment = critter:add_component('stonehearth:equipment')
   equipment:equip_item('stonehearth:pet_collar')
   town:add_pet(critter)

   ---[[
   --A few seconds in, set the time of day to right before sleepy time
   self:at(3000, function()
      stonehearth.calendar:set_time_unit_test_only({ hour = stonehearth.constants.sleep.BEDTIME_START - 1, minute = 58 })
   end)
   --Then, make someone super, super sleeply
   self:at(5000, function()
      radiant.entities.set_attribute(w2, 'sleepiness', stonehearth.constants.sleep.EXHAUSTION)
      radiant.entities.set_attribute(critter, 'sleepiness', stonehearth.constants.sleep.EXHAUSTION)
   end)
   --]]


end

return SleepTest

