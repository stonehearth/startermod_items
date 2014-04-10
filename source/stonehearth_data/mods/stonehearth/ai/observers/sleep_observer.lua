local calendar = stonehearth.calendar

local SleepObserver = class()
SleepObserver.version = 2

function SleepObserver:__init(entity)
   self._entity = entity

   self._attributes_component = entity:add_component('stonehearth:attributes')
   
   radiant.events.listen(calendar, 'stonehearth:hourly', self, self.on_hourly)
end

function SleepObserver:initialize(entity, json)
end

function SleepObserver:destroy()
   radiant.events.unlisten(calendar, 'stonehearth:hourly', self, self.on_hourly)
end

function SleepObserver:on_hourly(e)
   local sleepiness = self._attributes_component:get_attribute('sleepiness')

   -- todo, bias this to wake people up on sunrise if their sleepiness is low?
   if not radiant.entities.has_buff(self._entity, 'stonehearth:buffs:sleeping') then
      sleepiness = sleepiness + (100/16)
   end
   self._attributes_component:set_attribute('sleepiness', sleepiness)
end

function SleepObserver:destroy()
   radiant.events.unlisten(calendar, 'stonehearth:hourly', self, self.on_hourly)
end

return SleepObserver
