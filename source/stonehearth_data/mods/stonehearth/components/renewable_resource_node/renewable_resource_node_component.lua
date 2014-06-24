local RenewableResourceNodeComponent = class()

function RenewableResourceNodeComponent:initialize(entity, json)
   self._entity = entity
   self._calendar_constants = stonehearth.calendar:get_constants();
   
   self._original_description = self._entity:get_component('unit_info'):get_description()
   self._wait_text = self._original_description
   self._render_info = self._entity:add_component('render_info')

   self._resource = json.resource
   self._wait_text = json.wait_text
   self._renew_effect_name = json.renew_effect
   self._harvest_command_name = json.harvest_command --name of the cmd that harvests the resource
   self._harvest_overlay_effect = json.harvest_overlay_effect
   self._task_group_name = json.task_group_name
   self._harvest_tool = json.harvest_tool

   if json.renewal_time then
      local duration = string.sub(json.renewal_time, 1, -2)
      local time_unit = string.sub(json.renewal_time, -1, -1)

      if time_unit == 'd' then
         duration = duration * self._calendar_constants.hours_per_day
      elseif time_unit == 'm' then
         assert(false, 'durations under 1 hour are not supported for renewable resource nodes!')
      end

      self._renewal_time = tonumber(duration) -- how long until the resource respawns
   end
end

function RenewableResourceNodeComponent:get_task_group_name()
   return self._task_group_name
end

function RenewableResourceNodeComponent:get_harvest_tool()
   return self._harvest_tool
end

function RenewableResourceNodeComponent:get_harvest_overlay_effect()
   return self._harvest_overlay_effect
end

function RenewableResourceNodeComponent:spawn_resource(location)
   if self._resource then
      --Create the harvested entity and put it on the ground
      local item = radiant.entities.create_entity(self._resource)
      radiant.terrain.place_entity(item, location)

      --start the countdown to respawn.
      local render_info = self._entity:add_component('render_info')
      render_info:set_model_variant('depleted')
      self._renewal_countdown = self._renewal_time
      radiant.events.listen(stonehearth.calendar, 'stonehearth:hourly', self, self.on_hourly)

      --Change the description
      self._entity:get_component('unit_info'):set_description(self._wait_text)
   
      --Listen for renewal triggers, if relevant
      if self._renew_effect_name then
      end

      -- Fire an event to let everyone else know we've just been harvested
      radiant.events.trigger_async(self._entity, 'stonehearth:harvested', { entity = self._entity} )
      return item
   end
end


--- Reset the model to the default. Also, stop listening for effects
function RenewableResourceNodeComponent:_reset_model()
   self._render_info:set_model_variant('')
   if self._renew_effect then
      self._renew_effect:set_finished_cb(nil)
                        :set_trigger_cb(nil)
                        :stop()
      self._renew_effect = nil
   end
end

function RenewableResourceNodeComponent:renew(location)
   --If we have a renew effect associated, run it. If not, just swap the model.
   if self._renew_effect_name then
      assert(not self._renew_effect)
      self._renew_effect = radiant.effects.run_effect(self._entity, self._renew_effect_name)
      self._renew_effect:set_finished_cb(function()
            --- If we ran a renew effect but have no trigger to set the model, do so on finish
            self:_reset_model()
         end)

      self._renew_effect:set_trigger_cb(function(info)
            --- If the renew effect has a trigger in it to change the model, do so. 
            if info.event == "change_model" then
               self:_reset_model()
            end
         end)

      radiant.events.listen(self._renew_effect, 'stonehearth:on_effect_trigger', self, self.on_effect_trigger)
   else 
      self._render_info:set_model_variant('')
   end
  
   --Change the description
   self._entity:get_component('unit_info'):set_description(self._original_description)

   -- Fire an event to let everyone else know we're harvestable
   radiant.events.trigger_async(self._entity, 'stonehearth:is_harvestable', { entity = self._entity} )
end



function RenewableResourceNodeComponent:on_hourly()
   if self._renewal_countdown <= 0 then
      radiant.events.unlisten(stonehearth.calendar, 'stonehearth:hourly', self, self.on_hourly)
      self:renew()
   else 
      self._renewal_countdown = self._renewal_countdown - 1
   end
end

return RenewableResourceNodeComponent
