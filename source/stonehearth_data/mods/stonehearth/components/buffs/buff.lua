local Buff = class()

function Buff:__init(entity, arg1)
   self._entity = entity
   self._attribute_modifiers = {}
   if type(arg1) == 'string' then
      self:_initialize(arg1)
   else
      self:_restore(arg1)
   end
end

function Buff:_initialize(uri)
   -- can the ui just do a GET of this file, so we don't clone the data
   -- many, many, many times in the save file?
   self.__saved_variables = radiant.create_datastore(radiant.resources.load_json(uri))
   self._sv = self.__saved_variables:get_data()
   self._sv.uri = uri
   self:_create_buff()
end

function Buff:_restore(savestate)
   self.__saved_variables = savestate
   self._sv = self.__saved_variables:get_data()
   self:_create_buff()
end

function Buff:destroy()
   self._entity:add_component('stonehearth:buffs'):remove_buff(self._sv.uri)
   self:_destroy_effect()
   self:_destroy_duration()
   self:_destroy_modifiers()
   self:_destroy_injected_ai()
   self:_destroy_controller()
end

function Buff:_create_buff()
   self:_create_effect()
   self:_create_duration()
   self:_create_modifiers()
   self:_create_injected_ai()
   self:_create_controller()
end

function Buff:_create_controller()  
   assert(not self._controller)
   if self._sv.controller then
      self._controller = radiant.mods.load_script(self._sv.controller)()
      if self._controller.on_buff_added then
         self._controller:on_buff_added(self._entity, self)
      end
   end
end

function Buff:get_controller()
   if not self._controller then
      self:_create_controller()
   end
   return self._controller
end

function Buff:_destroy_controller()
   if self._controller then
      if self._controller.on_buff_removed then
         self._controller:on_buff_removed(self._entity, self)
      end
      self._controller = nil
   end
end

function Buff:_create_effect()   
   assert(not self._effect)
   if self._sv.effect then
      self._effect = radiant.effects.run_effect(self._entity, self._sv.effect)
   end
end

function Buff:_destroy_effect()
   if self._effect then
      self._effect:stop()
      self._effect = nil
   end
end

function Buff:_create_modifiers()   
   assert(#self._attribute_modifiers == 0)
   local modifiers = self._sv.modifiers
   if modifiers then
      local attributes = self._entity:add_component('stonehearth:attributes')
      for name, modifier in pairs(modifiers) do
         local attribute_modifier = attributes:modify_attribute(name)
         for type, value in pairs (modifier) do
            if type == 'multiply' then
               attribute_modifier:multiply_by(value)
            elseif type == 'add' then
               attribute_modifier:add_by(value)
            elseif type == 'min' then
               attribute_modifier:set_min(value)
            elseif type == 'max' then
               attribute_modifier:set_max(value)
            end
         end         
         table.insert(self._attribute_modifiers, attribute_modifier)
      end
   end
end

function Buff:_destroy_modifiers()
   if #self._attribute_modifiers > 0 then
      for i, modifier in ipairs(self._attribute_modifiers) do
         modifier:destroy()
      end
      self._attribute_modifiers = {}
   end
end


function Buff:_create_duration()
   local duration
   if self._sv.expire_time then
      -- expire_time is set to the game calendar time when the buff should
      -- expire.  if we're loading, we need to re-register the timer with
      -- this time rather than the duration
      duration = self._sv.expire_time - stonehearth.calendar:get_elapsed_time()
   elseif self._sv.duration then
      duration = self._sv.duration
   end
   if duration then
      self._timer = stonehearth.calendar:set_timer(duration, function()
            self:destroy()
         end)
      self._sv.expire_time = self._timer:get_expire_time()
      self.__saved_variables:mark_changed()
   end
end

function Buff:_destroy_duration()
   if self._timer then
      self._timer:destroy()
      self._timer = nil
   end   
end

function Buff:_create_injected_ai()
   assert(not self._injected_ai)
   local injected_ai = self._sv.injected_ai
   if injected_ai then
      self._injected_ai = stonehearth.ai:inject_ai(self._entity, injected_ai)
   end
end

function Buff:_destroy_injected_ai()
   if self._injected_ai then
      self._injected_ai:destroy()
      self._injected_ai = nil
   end
end   


function Buff:get_type()
   return self.type
end

function Buff:get_effect()
   return self.effect
end

function Buff:get_modifiers()
   return self.modifiers
end

function Buff:get_duration()
   return self.duration
end

function Buff:get_injected_ai()
   return self.injected_ai 
end

function Buff:get_render_info()
   return self.render
end

return Buff
