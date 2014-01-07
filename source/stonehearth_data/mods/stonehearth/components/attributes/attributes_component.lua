local AttributeModifier = require 'components.attributes.attribute_modifier'

local AttributesComponent = class()

function AttributesComponent:__init(entity, data_binding)
   self._entity = entity
   self._data_binding = data_binding
   self._attributes = {}
   self._modifiers = {}
   self._data_binding:update(self._attributes)
end

function AttributesComponent:extend(json)
   if json then
      for n, v in pairs(json) do
         self._attributes[n] = {}
         self._attributes[n].value = v
         self._attributes[n].effective_value = v
      end
   end
   self._data_binding:mark_changed()
end

function AttributesComponent:get_attribute(name)
   if not self._attributes[name] then
      self._attributes[name] = {
         value = 0
      }
   end

   if self._attributes[name].effective_value then
      return self._attributes[name].effective_value
   else
      return self._attributes[name].value
   end
end

function AttributesComponent:set_attribute(name, value)
   if not self._attributes[name] then
      self._attributes[name] = {
         value = 0
      }
   end

   if value ~= self._attributes[name].value then
      self._attributes[name].value = value
      self._attributes[name].effective_value = nil
      self:_recalculate(name)
   end
end

function AttributesComponent:modify_attribute(attribute)
   local modifier = AttributeModifier(self, attribute)
   
   if not self._modifiers[attribute] then
      self._modifiers[attribute] = {}
   end
   
   table.insert(self._modifiers[attribute], modifier)
   return modifier
end

function AttributesComponent:_remove_modifier(attribute, attribute_modifier)
   for i, modifier in ipairs(self._modifiers[attribute]) do
      if modifier == attribute_modifier then
         table.remove(self._modifiers[attribute], i)
         self:_recalculate(attribute)
         break
      end
   end
end

function AttributesComponent:_recalculate(name)
   local mult_factor = 1.0
   local add_factor = 0.0
   local min = nil
   local max = nil

   local original_value = self._attributes[name].effective_value

   for attribute_name, modifier_list in pairs(self._modifiers) do
      if attribute_name == name then
         for _, modifier in ipairs(modifier_list) do
            local mods = modifier:_get_modifiers();

            if mods['multiply'] then
               mult_factor = mult_factor * mods['multiply']
            end

            if mods['add'] then
               add_factor = add_factor + mods['add']
            end

            if mods['min'] then
               min = math.min(min, mods['min'])
            end

            if mods['max'] then
               max = math.max(max, mods['max'])
            end
         end   
      end
   end

   -- careful, order is important here!
   self._attributes[name].effective_value = self._attributes[name].value * mult_factor
   self._attributes[name].effective_value = self._attributes[name].effective_value + add_factor
   if min then
      self._attributes[name].effective_value = math.min(min, self._attributes[name].effective_value)
   end
   if max then
      self._attributes[name].effective_value = math.max(max, self._attributes[name].effective_value)
   end

   radiant.events.trigger(self._entity, 'stonehearth:attribute_changed:' .. name, { 
         value = self._attributes[name].effective_value
      })
   self._data_binding:mark_changed()
end

return AttributesComponent
