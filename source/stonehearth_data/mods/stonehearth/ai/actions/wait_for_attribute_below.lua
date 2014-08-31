local WaitForAttributeBelow = class()

--Waits for the specified attribute to hit or fall below a certain number

WaitForAttributeBelow.name = 'wait for attribute below threshold'
WaitForAttributeBelow.does = 'stonehearth:wait_for_attribute_below'
WaitForAttributeBelow.args = {
   attribute = 'string',   --the attribute to wait for
   value = 'number'        --the value we should be less than or equal to
}
WaitForAttributeBelow.version = 2
WaitForAttributeBelow.priority = 1

function WaitForAttributeBelow:start_thinking(ai, entity, args)
   self._ai = ai
   self._log = ai:get_log()
   self._entity = entity
   self._attribute = args.attribute
   self._below_value = args.value
   self._signaled = false
   self._attribute_listener = radiant.events.listen(entity, 'stonehearth:attribute_changed:' .. args.attribute, self, self._on_attribute_changed)

   self:_on_attribute_changed()
end

function WaitForAttributeBelow:stop_thinking(ai, entity, args)
   self:destroy()
end

function WaitForAttributeBelow:destroy()
   self._attribute_listener:destroy()
   self._attribute_listener = nil
end

function WaitForAttributeBelow:_on_attribute_changed()
   local value = radiant.entities.get_attribute(self._entity, self._attribute)
   self._log:spam('attribute:%s value:%d threshold:%d signaled:%s',
                  self._attribute, value, self._below_value, tostring(self._signaled))

   if value < self._below_value and not self._signaled then
      self._log:detail('marking ready!')
      self._signaled = true
      self._ai:set_think_output()
   elseif value >= self._below_value and self._signaled then
      self._log:detail('unmarking ready.')
      self._signaled = false
      self._ai:clear_think_output()
   end
end


return WaitForAttributeBelow