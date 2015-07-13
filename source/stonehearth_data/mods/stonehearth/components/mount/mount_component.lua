local Point3 = _radiant.csg.Point3
local log = radiant.log.create_logger('mount')

local MountComponent = class()

function MountComponent:initialize(entity, json)
   self._mountable_object = entity
   self._sv = self.__saved_variables:get_data()

   if not self._sv._initialized then
      local p = json.mount_offset
      local offset = p and Point3(p.x, p.y, p.z) or Point3.zero
      self._sv.mount_offset = offset
      self._sv.mounted_posture = json.mounted_posture
      self._sv.mounted_model_variant = json.mounted_model_variant
      self._sv._initialized = true
      self.__saved_variables:mark_changed()
   else
      self:_restore()
   end

end

function MountComponent:_restore()
   if self._sv.user then
      self:_trace_user(self._sv.user)
   end
end

function MountComponent:destroy()
   self:dismount()
end

function MountComponent:get_user()
   return self._sv.user
end

function MountComponent:is_in_use()
   return self._sv.user ~= nil
end

function MountComponent:get_mount_offset()
   return self._sv.mount_offset
end

function MountComponent:set_mount_offset(value)
   assert(not self:is_in_use(), 'not implemented')
   self._sv.mount_offset = value
   self.__saved_variables:mark_changed()
end

function MountComponent:get_mounted_posture()
   return self._sv.mounted_posture
end

function MountComponent:set_mounted_posture(value)
   assert(not self:is_in_use(), 'not implemented')
   self._sv.mounted_posture = value
   self.__saved_variables:mark_changed()
end

function MountComponent:get_mounted_model_variant()
   return self._sv.mounted_model_variant
end

function MountComponent:set_mounted_model_variant(value)
   assert(not self:is_in_use(), 'not implemented')
   self._sv.mounted_model_variant = value
   self.__saved_variables:mark_changed()
end

-- model_variant_delay is optional
function MountComponent:mount(user, model_variant_delay)
   model_variant_delay = model_variant_delay or 0

   if user == self._sv.user then
      return true
   end

   -- acquire a persistent lease or convert the existing lease into a persistent one
   if not stonehearth.ai:acquire_ai_lease(self._mountable_object, user, { persistent = true }) then
      return false
   end

   self:_save_state(user)

   local facing = radiant.entities.get_facing(self._mountable_object)
   local origin = radiant.entities.get_world_location(self._mountable_object)
   local offset = self:_get_worldspace_offset()
   local location = origin + offset
   local mob = self._sv.user:add_component('mob')

   mob:set_mob_collision_type(_radiant.om.Mob.NONE)
   radiant.entities.add_child(self._mountable_object, user, offset, true)

   -- must call after setting location
   self:_trace_user(user)

   if self._sv.mounted_posture then
      radiant.entities.set_posture(user, self._sv.mounted_posture)
   end

   if self._sv.mounted_model_variant then
      if model_variant_delay <= 0 then
         self:_set_model_variant(self._sv.mounted_model_variant)
      else
         self._model_variant_timer = stonehearth.calendar:set_timer('mount component', model_variant_delay, function()
               self:_set_model_variant(self._sv.mounted_model_variant)
               self:_destroy_model_variant_timer()
            end)
      end
   end

   return true
end

function MountComponent:dismount(set_egress_location)
   if set_egress_location == nil then
      set_egress_location = true
   end

   local user = self._sv.user

   self:_destroy_model_variant_timer()

   stonehearth.ai:release_ai_lease(self._mountable_object, user)

   if user and user:is_valid() then
      -- call before setting location
      self:_destroy_user_traces()

      local mob = user:add_component('mob')

      if set_egress_location then
         local facing = self._sv.saved_facing + 180
         local root_entity = radiant.entities.get_root_entity()
         radiant.entities.add_child(root_entity, user, self._sv.saved_location)
         mob:turn_to(facing)
      end

      mob:set_mob_collision_type(self._sv.saved_collision_type)

      if self._sv.mounted_posture then
         radiant.entities.unset_posture(user, self._sv.mounted_posture)
      end

      if self._sv.mounted_model_variant then
         self:_set_model_variant('')
      end
   end

   self:_clear_state()
   self:_destroy_user_destroy_trace()
end

function MountComponent:_set_model_variant(name)
   local render_info = self._mountable_object:add_component('render_info')
   render_info:set_model_variant(name)
end

function MountComponent:_trace_user(user)
   self:_trace_user_destroy(user)
   self:_trace_user_location(user)
end

function MountComponent:_destroy_user_traces()
   self:_destroy_user_destroy_trace()
   self:_destroy_user_location_trace()
end

function MountComponent:_trace_user_destroy(user)
   self._user_destroy_trace = radiant.events.listen(user, 'radiant:entity:pre_destroy', function()
         self:dismount()
      end)
end

function MountComponent:_destroy_user_destroy_trace()
   if self._user_destroy_trace then
      self._user_destroy_trace:destroy()
      self._user_destroy_trace = nil
   end
end

function MountComponent:_trace_user_location(user)
   self._user_location_trace = radiant.entities.trace_location(user, 'mount component')
      :on_changed(function()
            -- we must have decided to do something else, just dismount with minimal disruption
            self:dismount(false)
         end)
end

function MountComponent:_destroy_user_location_trace()
   if self._user_location_trace then
      self._user_location_trace:destroy()
      self._user_location_trace = nil
   end
end

function MountComponent:_destroy_model_variant_timer()
   if self._model_variant_timer then
      self._model_variant_timer:destroy()
      self._model_variant_timer = nil
   end
end

function MountComponent:_get_worldspace_offset()
   local facing = radiant.entities.get_facing(self._mountable_object)
   local offset = radiant.math.rotate_about_y_axis(self._sv.mount_offset, facing)
   return offset
end

function MountComponent:_save_state(user)
   local mob = user:add_component('mob')
   self._sv.user = user
   self._sv.saved_collision_type = mob:get_mob_collision_type()
   self._sv.saved_location = mob:get_world_location()
   self._sv.saved_facing = mob:get_facing()
   self.__saved_variables:mark_changed()
end

function MountComponent:_clear_state()
   self._sv.user = nil
   self._sv.saved_collision_type = nil
   self._sv.saved_location = nil
   self._sv.saved_facing = nil
   self.__saved_variables:mark_changed()
end

return MountComponent
