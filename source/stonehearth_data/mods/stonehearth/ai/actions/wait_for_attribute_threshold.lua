local event_service = stonehearth.events

local WaitForAttributeThreshold = class()
WaitForAttributeThreshold.name = 'wait for attribute threshold'
WaitForAttributeThreshold.does = 'stonehearth:wait_for_attribute_threshold'
WaitForAttributeThreshold.args = {
   attribute = 'string',      -- the attribute to wait for
   value = 'number',          -- the value that should be exceeded
}
WaitForAttributeThreshold.version = 2
WaitForAttributeThreshold.priority = 1

function WaitForAttributeThreshold:start_thinking(ai, entity, args)
   self._above_value = args.value
   local attribute = 'stonehearth:attribute_changed:' .. args.attribute
   radiant.events.listen(entity, attribute, self, self._attribute_changed)
end

function WaitForAttributeThreshold:stop_thinking(ai, entity)
   local attribute = 'stonehearth:attribute_changed:' .. args.attribute
   radiant.events.unlisten(entity, attribute, self, self._attribute_changed)
end

function WaitForAttributeThreshold:_attribute_changed(e)
   if e.value >= self._above_value then
      self._ai:set_think_output()
   else
      self._ai:clear_think_output()
   end
end

return WaitForAttributeThreshold
