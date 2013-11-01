local AttributeModifier = class() 

function AttributeModifier:__init(attributes_component, attribute)
   self._attribute = attribute
   self._attributes_component = attributes_component
   self._mods = {}
end

function AttributeModifier:multiply_by(value)
   self._mods['multiply'] = value
   self:_recalculate()
end

function AttributeModifier:add_by(value)
   self._mods['add'] = value
   self:_recalculate()
end

function AttributeModifier:set_min(value)
   self._mods['min'] = value
   self:_recalculate()
end

function AttributeModifier:set_max(value)
   self._mods['max'] = value
   self:_recalculate()
end

function AttributeModifier:_get_modifiers()
   return self._mods
end

function AttributeModifier:_recalculate()
   self._attributes_component:_recalculate(self._attribute)
end

function AttributeModifier:destroy()
   self._attributes_component:_remove_modifier(self._attribute, self)
end


return AttributeModifier