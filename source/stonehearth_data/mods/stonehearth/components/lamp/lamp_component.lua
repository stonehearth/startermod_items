local LampComponent = class()

function LampComponent:initialize(entity, json)
   self._entity = entity
   self._tracked_entities = {}

   self._sv = self.__saved_variables:get_data()
   if not self._sv.light_effect then
      self._sv.light_effect = json.light_effect  
      self.__saved_variables:mark_changed()
   end

   self:_check_light()

   radiant.events.listen(stonehearth.calendar, 'stonehearth:sunrise', self, self._light_off)
   radiant.events.listen(stonehearth.calendar, 'stonehearth:sunset', self, self._light_on)
end

function LampComponent:destroy()
   if self._running_effect then
      self._running_effect:stop()
      self._open_effect = nil
   end

   radiant.events.unlisten(stonehearth.calendar, 'stonehearth:sunrise', self, self._light_off)
   radiant.events.unlisten(stonehearth.calendar, 'stonehearth:sunset', self, self._light_on)
end

function LampComponent:_check_light()
   --Only light fires after dark
   local time_constants = stonehearth.calendar:get_constants()
   local curr_time = stonehearth.calendar:get_time_and_date()
   local should_light = curr_time.hour >= time_constants.event_times.sunset or
                        curr_time.hour < time_constants.event_times.sunrise 

   if should_light then
      self:_light_on()
   else
      self:_light_off()
   end
end

function LampComponent:_light_on()
   if self._sv.light_effect and not self._running_effect then
      self._running_effect = radiant.effects.run_effect(self._entity, self._sv.light_effect);
   end

   self._sv.lit = true;
   self.__saved_variables:mark_changed()
end

function LampComponent:_light_off()
   if self._running_effect then
      self._running_effect:stop()
      self._running_effect = nil
   end

   self._sv.lit = false;
   self.__saved_variables:mark_changed()
end


return LampComponent