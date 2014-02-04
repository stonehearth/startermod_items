local calendar = stonehearth.calendar
local time_constants = radiant.resources.load_json('/stonehearth/services/calendar/calendar_constants.json')

--[[
   Increases hunger attribute each hour.
   Hunger is a value that's usually between 0 and 100.
   At 0, a person has no desire to eat. 
   At 80, they start looking for something to eat. 
   At 120, they absolutely have to look for something to eat
   Different kinds of food satisfy hunger to a different degree.  
   Hunger can go over 100 and fall under 0. 
   Note: When an attribute is first created, its value is 0
]]
local HungerObserver = class()
HungerObserver.version = 2

function HungerObserver:__init(entity)
   self._entity = entity
   self._attributes_component = entity:add_component('stonehearth:attributes')
   radiant.events.listen(calendar, 'stonehearth:hourly', self, self.on_hourly)
   radiant.events.listen(entity, 'stonehearth:attribute_changed:hunger', self, self._hunger_changed)
end

function HungerObserver:_hunger_changed()
   local hunger = self._attributes_component:get_attribute('hunger')

   local starving = hunger >= 120
   local hungry = hunger >= 80

   if starving and not self._showing_starving then
      self._showing_starving = true
      radiant.entities.add_buff(self._entity, 'stonehearth:buffs:starving')
   elseif not starving and self._showing_starving then
      self._showing_hungry = false
      radiant.entities.remove_buff(self._entity, 'stonehearth:buffs:starving')
   end
   if hungry then
      radiant.entities.think(self._entity, '/stonehearth/data/effects/thoughts/hungry')
   else
      radiant.entities.unthink(self._entity, '/stonehearth/data/effects/thoughts/hungry')
   end
end

function HungerObserver:on_hourly(e)
   local hunger = self._attributes_component:get_attribute('hunger')
   local hours_till_showing_hungry = time_constants.hours_per_day

   -- TODO consider slowing hunger while sleepy
   hunger = hunger + ( 100 / hours_till_showing_hungry)
   self._attributes_component:set_attribute('hunger', hunger)
end

return HungerObserver