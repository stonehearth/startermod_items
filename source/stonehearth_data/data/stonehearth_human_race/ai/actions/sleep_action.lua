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

   if calendar.hour > 21 or calendar.hour < 6 then
      self._sleepiness = 100
   else 
      self._sleepiness = 0
   end

   self._ai:set_action_priority(self, self._sleepiness)
   
end

function SleepAction:run(ai, entity)
   ai:execute('stonehearth.activities.run_effect', 'yawn')
   ai:execute('stonehearth.activities.run_effect', 'goto_sleep')
   ai:execute('stonehearth.activities.run_effect', 'sleep')
end

return SleepAction