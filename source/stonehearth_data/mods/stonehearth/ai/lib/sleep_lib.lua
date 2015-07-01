local constants = require 'constants'

local SleepLib = class()

function SleepLib.get_sleep_parameters(entity, bed_entity)
   -- calculate sleep duration in minutes
   local sleep_duration = 60
   local rested_sleepiness = 0
   local attributes_component = entity:get_component('stonehearth:attributes')
   
   if attributes_component then
      local sleep_duration_attribute = attributes_component:get_attribute('sleep_duration')
      if sleep_duration_attribute then
         sleep_duration = radiant.math.round(sleep_duration_attribute)
      end
   end

   if not bed_entity then
      -- sleeping on the ground
      sleep_duration = sleep_duration * 2
      rested_sleepiness = constants.sleep.MIN_SLEEPINESS
   end

   local sleep_duration = sleep_duration .. 'm'
   return sleep_duration, rested_sleepiness
end

return SleepLib
