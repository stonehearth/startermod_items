local calendar = stonehearth.calendar
local GrowingComponent = class()

function GrowingComponent:__init(entity, data_binding)
   self._entity = entity
   self._data = data_binding:get_data()

   self._data_binding = data_binding
   self._data_binding:mark_changed()

end

function GrowingComponent:extend(json)
   if json then

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
         self._render_info:set_model_variant(self._growth_stages[1])
      end

      if self._data.curr_stage < #self._growth_stages then
         --Every hour, check if we should be growing by some period
         radiant.events.listen(calendar, 'stonehearth:hourly', self, self.on_hourly)
      end

      self._data_binding:mark_changed()
   end
end

function GrowingComponent:on_hourly()
   if self._data.growth_countdown <= 0 then
      self:grow()
   else 
      self._data.growth_countdown = self._data.growth_countdown - 1
   end
end

--- Do whatever is necessary to get to the next step of growth
function GrowingComponent:grow()
   --TODO: run growth effect, if we have one
   self._data.curr_stage = self._data.curr_stage + 1
   self._render_info:set_model_variant(self._growth_stages[self._data.curr_stage])
   --TODO: use events to tell crop component to suck nutrients out of the soil?
   if self._data.curr_stage >= #self._growth_stages then
      self:stop_growing()
   end
end

--- When we're done growing, stop listening and fire an event
function GrowingComponent:stop_growing()
   radiant.events.unlisten(calendar, 'stonehearth:hourly', self, self.on_hourly)
   radiant.events.trigger(self._entity, 'stonehearth:growth_complete', {entity = self._entity})
end

return GrowingComponent