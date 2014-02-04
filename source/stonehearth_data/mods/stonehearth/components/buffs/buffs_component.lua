Buff = require 'components.buffs.buff'
AttributeModifier = require 'components.attributes.attribute_modifier'
local calendar = stonehearth.calendar

local BuffsComponent = class()

function BuffsComponent:__init(entity, data_binding)
   self._data_binding = data_binding
   self._entity = entity
   self._attributes_component = entity:add_component('stonehearth:attributes')
   self._buffs = {}
   self._attribute_modifiers = {}
   self._injected_ais = {}
   self._data_binding:update(self._buffs)
   self._calendar_constants = calendar:get_constants()
end

function BuffsComponent:add_buff(uri)
   assert(not string.find(uri, '%.'), 'tried to add a buff with a uri containing "." Use an alias instead')

   local buff
   if self._buffs[uri] then
      --xxx, check a policy for status vs. stacking buffs
      buff = self._buffs[uri]
   else 
      buff = Buff(self._entity, uri)
      self._buffs[uri] = buff
      self:_apply_duration(uri, buff)
      self:_apply_modifiers(uri, buff)
      self:_inject_ai(uri, buff)
      self._data_binding:mark_changed()
   end

   return buff
end

function BuffsComponent:has_buff(uri)
   return self._buffs[uri] ~= nil
end

function BuffsComponent:remove_buff(uri)
   self._buffs[uri] = nil

   self:_remove_modifiers(uri);
   self:_uninject_ai(uri)

   self._data_binding:mark_changed()
end

function BuffsComponent:_apply_modifiers(uri, buff)
   local modifiers = buff:get_modifiers()

   if modifiers then

      self._attribute_modifiers[uri] = {}
      for name, modifier in pairs(modifiers) do
         local attribute_modifier = self._attributes_component:modify_attribute(name)

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
         
         table.insert(self._attribute_modifiers[uri], attribute_modifier)
      end
   end
end

function BuffsComponent:_remove_modifiers(uri)
   if self._attribute_modifiers[uri] then
      for i, modifier in ipairs(self._attribute_modifiers[uri]) do
         modifier:destroy()
      end
      self._attribute_modifiers[uri] = nil
   end
end

function BuffsComponent:_apply_duration(uri, buff)
   local duration = buff:get_duration() 

   if duration then 
      -- convert time string into seconds and set a callback to remove the buff
      local buff_duration = string.sub(duration, 1, -2)
      local time_unit = string.sub(duration, -1, -1)

      if time_unit == 'h' then
         buff_duration = buff_duration * (self._calendar_constants.minutes_per_hour * self._calendar_constants.seconds_per_minute)
      elseif time_unit == 'm' then
         buff_duration = buff_duration * self._calendar_constants.seconds_per_minute
      end

      calendar:set_timer(0, 0, buff_duration, function()
         self:remove_buff(uri)
      end)
   end
end

function BuffsComponent:_inject_ai(uri, buff)
   local ai = buff:get_injected_ai();

   if ai then 
      local ai_service = stonehearth.ai
      local injected_ai_token = ai_service:inject_ai(self._entity, ai)
      self._injected_ais[uri] = injected_ai_token
   end
end

function BuffsComponent:_uninject_ai(uri)
   if self._injected_ais[uri] then
      self._injected_ais[uri]:destroy()
      self._injected_ais[uri] = nil
   end
end


return BuffsComponent
