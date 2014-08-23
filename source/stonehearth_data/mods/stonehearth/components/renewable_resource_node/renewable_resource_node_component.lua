local RenewableResourceNodeComponent = class()

function RenewableResourceNodeComponent:initialize(entity, json)
   self._entity = entity

   self._calendar_constants = stonehearth.calendar:get_constants();
   
   self._original_description = self._entity:get_component('unit_info'):get_description()
   self._unripe_description = self._original_description
   self._render_info = self._entity:add_component('render_info')

   self._resource = json.resource
   self._unripe_description = json.unripe_description
   self._renew_effect_name = json.renew_effect
   self._harvest_command_name = json.harvest_command --name of the cmd that harvests the resource
   self._harvest_overlay_effect = json.harvest_overlay_effect
   self._task_group_name = json.task_group_name
   self._harvest_tool = json.harvest_tool

   if json.renewal_time then
      self._renewal_time = stonehearth.calendar:parse_duration(json.renewal_time)
   end

   self._sv = self.__saved_variables:get_data()
   if not self._sv.initialized then
      self._sv.harvestable = true
      self._sv.initialized = true
      self.__saved_variables:mark_changed()
   else
      self:_restore()
   end
end

function RenewableResourceNodeComponent:_restore()
   if self._sv.next_renew_time then
      local duration = stonehearth.calendar:get_seconds_until(self._sv.next_renew_time)
      self:_start_renew_timer(duration)
   end
end

function RenewableResourceNodeComponent:is_harvestable()
   return self._sv.harvestable
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
      local pt = radiant.terrain.find_placement_point(location, 0, 2)
      radiant.terrain.place_entity(item, pt)

      --start the countdown to respawn.
      local render_info = self._entity:add_component('render_info')
      render_info:set_model_variant('depleted')
      self:_start_renew_timer(self._renewal_time)

      --Change the description
      if self._unripe_description then
         self._entity:get_component('unit_info'):set_description(self._unripe_description)
      end
   
      --Listen for renewal triggers, if relevant
      if self._renew_effect_name then
      end

      self._sv.harvestable = false
      self.__saved_variables:mark_changed()

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

   self._sv.harvestable = true
   self.__saved_variables:mark_changed()
end

function RenewableResourceNodeComponent:_start_renew_timer(duration)
   self:_stop_renew_timer()

   self._renew_timer = stonehearth.calendar:set_timer(duration,
      function ()
         self:_stop_renew_timer()
         self:renew()
      end
   )

   self._sv.next_renew_time = self._renew_timer:get_expire_time()
   self.__saved_variables:mark_changed()
end

function RenewableResourceNodeComponent:_stop_renew_timer()
   if self._renew_timer then
      self._renew_timer:destroy()
      self._renew_timer = nil
   end

   self._sv.next_renew_time = nil
   self.__saved_variables:mark_changed()
end

return RenewableResourceNodeComponent
