local MicroWorld = require 'lib.micro_world'

local SleepTest = class(MicroWorld)
--[[
   Instantiate a worker and a bed. Set the time of day to night time, so
   the worker will run to his bed and go to sleep.
]]

function SleepTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   self:place_citizen(-8, 8)
   self:place_citizen(8, 8)
   self:place_item('stonehearth.comfy_bed', 0, 8)
   self:place_item('stonehearth.comfy_bed', 0, 0)
   local tree = self:place_tree(-12, -12)

   ---[[
   --500ms seconds in, set the time of day to right before sleepy time
   self:at(500, function()
      --stonehearth_calendar.set_time(0, 45, 20)
   end)
   --]]

   ---[[
   --5000ms seconds in, set the time back to day
   self:at(10000, function()
      --stonehearth_calendar.set_time(0, 50, 11)
   end)
   --]]

end

return SleepTest

