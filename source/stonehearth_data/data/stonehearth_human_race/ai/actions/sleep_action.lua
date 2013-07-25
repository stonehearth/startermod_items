--[[
   Need a more robust implementation later. For now,
   go to sleep at 9pm and wake up at 6am
]]

-- make sure the calendar is loaded
radiant.mods.require('stonehearth_calendar') 

local SleepAction = class()

SleepAction.name = 'stonehearth.actions.sleep'
SleepAction.does = 'stonehearth.activities.top'
SleepAction.priority = 0

local HOURS_UNTIL_SLEEPY = 2;

function SleepAction:__init(ai, entity)
   radiant.check.is_entity(entity)
   self._entity = entity         --the game character
   self._ai = ai

   self._sleepiness = 0

   radiant.events.listen('radiant.events.calendar.hourly', self)
end

SleepAction['radiant.events.calendar.hourly'] = function(self, calendar)
   radiant.log.info('adjusting sleepiness')

   if calendar.hour > 20 or calendar.hour < 6 then
      self._sleepiness = 100
   else 
      self._sleepiness = 0
   end

   self._ai:set_action_priority(self, self._sleepiness)
   
end

function SleepAction:run(ai, entity)
   -- find a bed
   local inventory = radiant.mods.require('/stonehearth_inventory/')
   local faction = self._entity:get_component('unit_info'):get_faction()
   local i = inventory.get_inventory(faction)

   -- the filter function to find a bed
   local filter_fn = function(item)
      local bed = item:get_component('stonehearth_sleep_system:bed')
      return bed ~= nil
   end

   -- save the path to the bed once we find it
   local path_to_bed
   local solved_cb = function(path)
      path_to_bed = path
      ai:resume()
   end

   -- go find the path to the bed
   i:find_path_to_item(entity, solved_cb, filter_fn)
   ai:suspend()

   -- follow the path
   ai:execute('stonehearth.activities.follow_path', path_to_bed)

   -- goto sleep
   ai:execute('stonehearth.activities.run_effect', 'yawn')
   ai:execute('stonehearth.activities.run_effect', 'goto_sleep')
   ai:execute('stonehearth.activities.run_effect', 'sleep')
end

return SleepAction