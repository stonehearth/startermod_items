local calendar = stonehearth.calendar
local GrowingComponent = class()



function GrowingComponent:__create(entity, json)
   self._entity = entity
   self._data = {}

   
   if json.growth_period then
      local duration = string.sub(json.growth_period, 1, -2)
      local time_unit = string.sub(json.growth_period, -1, -1)

      if time_unit == 'd' then
         duration = duration * self._calendar_constants.hours_per_day
      elseif time_unit == 'm' then
         assert(false, 'durations under 1 hour are not supported for renewable resource nodes!')
      end
      --Otherwise, we assume it's hours
      self._growth_period = tonumber(duration)
      self._data.growth_countdown = self._growth_period
   end

   if json.growth_stages then
      self._growth_stages = json.growth_stages
      self._data.curr_stage = 1

      self._render_info = self._entity:add_component('render_info')
      self:_set_stage()
      
      if self._data.curr_stage < #self._growth_stages then
         --Every hour, check if we should be growing by some period
         radiant.events.listen(calendar, 'stonehearth:hourly', self, self.on_hourly)
      end
   end
   
   self.__savestate = radiant.create_datastore(self._data)
   self.__savestate:mark_changed()
end

function GrowingComponent:__destroy()
   radiant.events.unlisten(calendar, 'stonehearth:hourly', self, self.on_hourly)
end

--- Every hour, check if we should be growing
function GrowingComponent:on_hourly()
   if self._data.growth_countdown <= 0 then
      self:grow()
   else 
      self._data.growth_countdown = self._data.growth_countdown - 1
      self.__savestate:mark_changed()
   end
end

--- Do whatever is necessary to get to the next step of growth
function GrowingComponent:grow()
   --TODO: run growth effect, if we have one
   self._data.curr_stage = self._data.curr_stage + 1
   self:_set_stage()

   --TODO: use events to tell crop component to suck nutrients out of the soil?
   --TODO: stop growth if some condition is not met
   radiant.events.trigger(self._entity, 'stonehearth:growing', {entity = self._entity, stage = self._growth_stages[self._data.curr_stage].model_name})
   if self._data.curr_stage >= #self._growth_stages then
      self:stop_growing()   
   end
   self.__savestate:mark_changed()
end

function GrowingComponent:_set_stage()
   local stage_data = self._growth_stages[self._data.curr_stage]
   local stage_name = stage_data.model_name
   self._render_info:set_model_variant(stage_name)

   radiant.entities.set_name(self._entity, stage_data.name)
   radiant.entities.set_description(self._entity, stage_data.description)
end

--- When we're done growing, stop listening and fire an event
function GrowingComponent:stop_growing()
   radiant.events.unlisten(calendar, 'stonehearth:hourly', self, self.on_hourly)
   radiant.events.trigger(self._entity, 'stonehearth:growth_complete', {entity = self._entity})
end

return GrowingComponent