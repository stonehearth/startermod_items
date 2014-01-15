local event_service = stonehearth.events

local WaitForAttributeAbove = class()
WaitForAttributeAbove.name = 'wait for attribute threshold'
WaitForAttributeAbove.does = 'stonehearth:wait_for_attribute_above'
WaitForAttributeAbove.args = {
   attribute = 'string',      -- the attribute to wait for
   value = 'number',          -- the value that should be exceeded
}
WaitForAttributeAbove.version = 2
WaitForAttributeAbove.priority = 1

function WaitForAttributeAbove:start_thinking(ai, entity, args)
   self._ai = ai
   self._above_value = args.value
   self._signaled = false
   local attribute = 'stonehearth:attribute_changed:' .. args.attribute
   radiant.events.listen(entity, attribute, self, self._attribute_changed)
end

function WaitForAttributeAbove:stop_thinking(ai, entity, args)
   local attribute = 'stonehearth:attribute_changed:' .. args.attribute
   radiant.events.unlisten(entity, attribute, self, self._attribute_changed)
end

function WaitForAttributeAbove:_attribute_changed(e)
   if e.value >= self._above_value and not self._signaled then
      self._signaled = true
      self._ai:set_think_output()
   elseif e.value < self._above_value and self._signaled then
      self._signaled= false
      self._ai:clear_think_output()
   end
end

return WaitForAttributeAbove
