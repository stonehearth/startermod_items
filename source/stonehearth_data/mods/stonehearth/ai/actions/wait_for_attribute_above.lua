local WaitForAttributeAbove = class()
WaitForAttributeAbove.name = 'wait for attribute above threshold'
WaitForAttributeAbove.does = 'stonehearth:wait_for_attribute_above'
WaitForAttributeAbove.args = {
   attribute = 'string',      -- the attribute to wait for
   value = 'number',          -- the value that should be exceeded
}
WaitForAttributeAbove.version = 2
WaitForAttributeAbove.priority = 1

function WaitForAttributeAbove:start_thinking(ai, entity, args)
   self._ai = ai
   self._log = ai:get_log()
   self._entity = entity
   self._attribute = args.attribute
   self._above_value = args.value
   self._signaled = false

   local attribute = 'stonehearth:attribute_changed:' .. args.attribute
   radiant.events.listen(entity, attribute, self, self._on_attribute_changed)
   self:_on_attribute_changed()
end

function WaitForAttributeAbove:stop_thinking(ai, entity, args)
   local attribute = 'stonehearth:attribute_changed:' .. args.attribute
   radiant.events.unlisten(entity, attribute, self, self._on_attribute_changed)
end

function WaitForAttributeAbove:_on_attribute_changed()
   local value = radiant.entities.get_attribute(self._entity, self._attribute)
   self._log:spam('attribute:%s value:%d threshold:%d signaled:%s',
                  self._attribute, value, self._above_value, tostring(self._signaled))

   if value >= self._above_value and not self._signaled then
      self._log:detail('marking ready!')
      self._signaled = true
      self._ai:set_think_output()
   elseif value < self._above_value and self._signaled then
      self._log:detail('unmarking ready.')
      self._signaled = false
      self._ai:clear_think_output()
   end
end

return WaitForAttributeAbove
