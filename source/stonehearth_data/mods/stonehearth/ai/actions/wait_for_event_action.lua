local WaitForEvent = class()

WaitForEvent.name = 'wait for the target event'
WaitForEvent.does = 'stonehearth:wait_for_event'
WaitForEvent.args = {
   event_name = 'string'  --event we're waiting for, target always this entity
}
WaitForEvent.verson = 2
WaitForEvent.priority = 1

function WaitForEvent:start_thinking(ai, entity, args)
   self._ai = ai
   self._entity = entity
   self._event_listener = radiant.events.listen(entity, args.event_name, self, self._on_event)
end

function WaitForEvent:_on_event(e)
   if self._entity then
      self._ai:set_think_output()
   end
end

function WaitForEvent:stop_thinking(ai, entity, args)
   self:destroy()
end

function WaitForEvent:destroy()
   if self._event_listener then
      self._event_listener:destroy()
      self._event_listener = nil
   end
end

return WaitForEvent