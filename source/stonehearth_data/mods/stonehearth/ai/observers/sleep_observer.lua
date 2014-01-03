local calendar = require 'services.calendar.calendar_service'

local SleepObserver = class()

SleepObserver.does = 'stonehearth:top'
SleepObserver.version = 1
SleepObserver.priority = 0

function SleepObserver:__init(entity)
   self._entity = entity
   self._attributes_component = entity:add_component('stonehearth:attributes')
   radiant.events.listen(calendar, 'stonehearth:hourly', self, self.on_hourly)
end

function SleepObserver:on_hourly(e)
   local sleepiness = self._attributes_component:get_attribute('sleepiness')

   -- todo, bias this to wake people up on sunrise if their sleepiness is low?
   if radiant.entities.has_buff(self._entity, 'stonehearth:buffs:sleeping') then
      --sleepiness = math.max(0, sleepiness - 25)
      sleepiness = 0
   else 
      sleepiness = sleepiness + (100/16)
   end

   self._attributes_component:set_attribute('sleepiness', sleepiness)
end

return SleepObserver