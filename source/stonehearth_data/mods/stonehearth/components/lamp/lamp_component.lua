local LampComponent = class()

function LampComponent:initialize(entity, json)
   self._entity = entity
   self._tracked_entities = {}
   self._render_info = self._entity:add_component('render_info')
   
   self._sv.light_policy = json.light_policy or nil

   self._sv = self.__saved_variables:get_data()
   if not self._sv.light_effect then
      self._sv.light_effect = json.light_effect  
      self.__saved_variables:mark_changed()
   end

   self:_check_light()

   if not self._sv.on then
      self._sunrise_listener = radiant.events.listen(stonehearth.calendar, 'stonehearth:sunrise', self, self._light_off)
      self._sunset_listener = radiant.events.listen(stonehearth.calendar, 'stonehearth:sunset', self, self._light_on)
   end
end

function LampComponent:destroy()
   if self._running_effect then
      self._running_effect:stop()
      self._open_effect = nil
   end

   self._sunset_listener:destroy()
   self._sunset_listener = nil

   self._sunrise_listener:destroy()
   self._sunrise_listener = nil
end

function LampComponent:_check_light()
   --Only light fires after dark
   local time_constants = stonehearth.calendar:get_constants()
   local curr_time = stonehearth.calendar:get_time_and_date()
   local should_light = curr_time.hour >= time_constants.event_times.sunset or
                        curr_time.hour < time_constants.event_times.sunrise 

   if self._sv.light_policy == "always_on" then
      should_light = true
   end
   
   if should_light then
      self:_light_on()
   else
      self:_light_off()
   end
end

function LampComponent:_light_on()
   self._render_info:set_model_variant('lamp_on')
   if self._sv.light_effect and not self._running_effect then
      self._running_effect = radiant.effects.run_effect(self._entity, self._sv.light_effect);
   end

   self._sv.lit = true;
   self.__saved_variables:mark_changed()
end

function LampComponent:_light_off()
   self._render_info:set_model_variant('')
   if self._running_effect then
      self._running_effect:stop()
      self._running_effect = nil
   end

   self._sv.lit = false;
   self.__saved_variables:mark_changed()
end


return LampComponent