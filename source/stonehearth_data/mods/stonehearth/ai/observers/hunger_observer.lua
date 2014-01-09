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

HungerObserver.does = 'stonehearth:top'
HungerObserver.version = 1
HungerObserver.priority = 0

function HungerObserver:__init(entity)
   self._entity = entity
   self._attributes_component = entity:add_component('stonehearth:attributes')
   radiant.events.listen(calendar, 'stonehearth:hourly', self, self.on_hourly)
end

function HungerObserver:on_hourly(e)

   local hunger = self._attributes_component:get_attribute('hunger')
   local hours_till_hungry = time_constants.hours_per_day

   -- TODO consider slowing hunger while sleepy
   hunger = hunger + (100/hours_till_hungry)

   self._attributes_component:set_attribute('hunger', hunger)
end

return HungerObserver