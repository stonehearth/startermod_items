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
   self._below_value = args.value
   self._signaled = false
   local attribute = 'stonehearth:attribute_changed:' .. args.attribute
   radiant.events.listen(entity, attribute, self, self._on_attribute_changed)

   if radiant.entities.get_attribute(entity, args.attribute) <= args.value then
      self._signaled = true
      self._ai:set_think_output()
   end
end

function WaitForAttributeBelow:stop_thinking(ai, entity, args)
   local attribute = 'stonehearth:attribute_changed:' .. args.attribute
   radiant.events.unlisten(entity, attribute, self, self._on_attribute_changed)
end

function WaitForAttributeBelow:_on_attribute_changed(e)
   if e.value <= self._below_value and not self._signaled then
      self._signaled = true
      self._ai:set_think_output()
   elseif e.value > self._below_value and self._signaled then
      self._signaled = false
      self._ai:clear_think_output()
   end 
end

return WaitForAttributeBelow