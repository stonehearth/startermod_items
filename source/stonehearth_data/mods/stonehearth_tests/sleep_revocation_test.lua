local MicroWorld = require 'lib.micro_world'

local SleepRevocationTest = class(MicroWorld)
--[[
   Instantiate a worker and a bed. Set the time of day to night time, so
   the worker will run to his bed and go to sleep.
]]

function SleepRevocationTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   self:place_citizen(-8, 8)
   local bed = self:place_item('stonehearth:comfy_bed', 0, 0)

   local calendar = radiant.mods.load('stonehearth').calendar
   calendar:set_time(22, 59, 0)
   local move_distance = 5
   radiant.events.listen('radiant:events:calendar:hourly', function(event_name, time) 
      if time.hour == 3 then
         local bed_location = bed:get_component('mob'):get_grid_location()
         bed_location.x = bed_location.x + move_distance
         move_distance = move_distance * -1
         bed:get_component('mob'):set_location_grid_aligned(bed_location)
      end
   end)
   ---[[
   --500ms seconds in, set the time of day to right before sleepy time
   self:at(500, function()
   end)
   --]]

   ---[[
   --5000ms seconds in, set the time back to day
   self:at(10000, function()
      --stonehearth_calendar.set_time(0, 50, 11)
   end)
   --]]

end

return SleepRevocationTest

