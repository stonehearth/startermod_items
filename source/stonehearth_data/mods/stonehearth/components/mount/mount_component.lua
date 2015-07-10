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

function MountComponent:mount(user)
   if user == self._sv.user then
      return
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
      local render_info = self._mountable_object:add_component('render_info')
      render_info:set_model_variant(self._sv.mounted_model_variant)
   end
end

function MountComponent:dismount()
   local user = self._sv.user

   if user and user:is_valid() then
      -- call before setting location
      self:_destroy_user_traces()

      local mob = user:add_component('mob')
      local facing = self._sv.saved_facing + 180

      local root_entity = radiant.entities.get_root_entity()
      radiant.entities.add_child(root_entity, user, self._sv.saved_location)
      mob:turn_to(facing)
      mob:set_mob_collision_type(self._sv.saved_collision_type)

      if self._sv.mounted_posture then
         radiant.entities.unset_posture(user, self._sv.mounted_posture)
      end

      if self._sv.mounted_model_variant then
         local render_info = self._mountable_object:add_component('render_info')
         render_info:set_model_variant('')
      end
   end

   self:_clear_state()
   self:_destroy_user_destroy_trace()
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
            -- we must have decided to do something else, just dismount
            self:dismount()
         end)
end

function MountComponent:_destroy_user_location_trace()
   if self._user_location_trace then
      self._user_location_trace:destroy()
      self._user_location_trace = nil
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
