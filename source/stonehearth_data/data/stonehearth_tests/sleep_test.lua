
local MicroWorld = require 'stonehearth_tests.lib.micro_world'
local stonehearth_calendar = radiant.mods.require('stonehearth_calendar')

local SleepTest = class(MicroWorld)
--[[
   Instantiate a worker and a bed. Set the time of day to night time, so
   the worker will run to his bed and go to sleep.
]]

function SleepTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local worker = self:place_citizen(12, 12)
   local bed = self:place_item('/stonehearth_items/comfy_bed', -10, -10)

   ---[[
   --500ms seconds in, set the time of day to right before sleepy time
   self:at(500, function()
      stonehearth_calendar.set_time(0, 45, 20)
   end)
   --]]
end

return SleepTest

