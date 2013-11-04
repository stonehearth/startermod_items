Buff = require 'components.buffs.buff'
AttributeModifier = require 'components.attributes.attribute_modifier'

local BuffsComponent = class()

function BuffsComponent:__init(entity, data_binding)
   self._data_binding = data_binding
   self._entity = entity
   self._attributes_component = entity:add_component('stonehearth:attributes')
   self._buffs = {}
   self._attribute_modifiers = {}
   self._data_binding:update(self._buffs)
end

function BuffsComponent:add_buff(uri)
   assert(not string.find(uri, '%.'), 'tried to add a buff with a uri containing "." Use an alias instead')

   if self._buffs[uri] then
      --xxx, check a policy for status vs. stacking buffs
      self:remove_buff(uri)
   end

   local buff = Buff(self._entity, uri)
   self._buffs[uri] = buff
   
   self:apply_modifiers(uri, buff)
   self._data_binding:mark_changed()
   
   return buff
end

function BuffsComponent:has_buff(uri)
   return self._buffs[uri] ~= nil
end

function BuffsComponent:apply_modifiers(uri, buff)
   if not buff.modifiers then
      return
   end

   local attributes
   for name, modifier in pairs(buff.modifiers) do
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

      self._attribute_modifiers[uri] = attribute_modifier
      
   end
   
end

function BuffsComponent:remove_buff(uri)
   self._buffs[uri] = nil

   if self._attribute_modifiers[uri] then
      self._attribute_modifiers[uri]:destroy()
      self._attribute_modifiers[uri] = nil
   end

   self._data_binding:mark_changed()
end

return BuffsComponent
