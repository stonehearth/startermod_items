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

      --TODO: expand this so we can start with an unharvestable item with a timer to first harvestable
      
      self._sv.harvestable = true
      self._sv.initialized = true
      self.__saved_variables:mark_changed()
   else
      radiant.events.listen(radiant, 'radiant:game_loaded', function(e)
         self:_restore()
         --If we're harvestable on load, fire the harvestable event again,
         --in case we need to reinitialize tasks and other nonsavables on the event
         if self._sv.harvestable then
            radiant.events.trigger(self._entity, 'stonehearth:on_renewable_resource_renewed', {target = self._entity, available_resource = self._resource})
         end
         return radiant.events.UNLISTEN
      end)

      
   end
end

function RenewableResourceNodeComponent:_restore()
   if self._sv.renew_timer then
      self._sv.renew_timer:bind(function()
            self:renew()
         end)
   end
end

function RenewableResourceNodeComponent:destroy()
   self:_stop_renew_timer()
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

function RenewableResourceNodeComponent:spawn_resource(owner, location)
   if self._resource then
      --Create the harvested entity and put it on the ground
      local item = radiant.entities.create_entity(self._resource, { owner = owner })

      local pt = radiant.terrain.find_placement_point(location, 0, 2)
      radiant.terrain.place_entity(item, pt)

      -- add it to the inventory of the owner
      stonehearth.inventory:get_inventory(owner)
                              :add_item(item)

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

function RenewableResourceNodeComponent:renew()
   self:_stop_renew_timer()

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

   --Let whoever's listening know that we have now respawned our resources
   radiant.events.trigger(self._entity, 'stonehearth:on_renewable_resource_renewed', {target = self._entity, available_resource = self._resource})

   self.__saved_variables:mark_changed()
end

function RenewableResourceNodeComponent:_start_renew_timer(duration)
   self:_stop_renew_timer()

   self._sv.renew_timer = stonehearth.calendar:set_timer("RenewableResourceNodeComponent renew", duration,
      function ()
         self:renew()
      end
   )

   self.__saved_variables:mark_changed()
end

function RenewableResourceNodeComponent:_stop_renew_timer()
   if self._sv.renew_timer then
      self._sv.renew_timer:destroy()
      self._sv.renew_timer = nil
   end

   self.__saved_variables:mark_changed()
end

return RenewableResourceNodeComponent
