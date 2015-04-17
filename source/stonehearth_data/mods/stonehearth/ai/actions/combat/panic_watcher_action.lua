local Entity = _radiant.om.Entity
local log = radiant.log.create_logger('combat')

local PanicWatcher = class()

PanicWatcher.name = 'panic watcher'
PanicWatcher.does = 'stonehearth:combat:panic_watcher'
PanicWatcher.args = {}
PanicWatcher.think_output = {
   threat = Entity,
}
PanicWatcher.version = 2
PanicWatcher.priority = 1
PanicWatcher.realtime = true

function PanicWatcher:start_thinking(ai, entity, args)
   self._entity = entity
   self._ai = ai

   local threat = self:_get_panicking_from()

   if threat then
      self:_set_panicking(threat)
   else
      self._panicking_listener = radiant.events.listen(self._entity, 'stonehearth:combat:panic_changed', self, self._on_check_panicking)
   end
end

function PanicWatcher:stop_thinking(ai, entity, args)
   if self._panicking_listener then
      self._panicking_listener:destroy()
      self._panicking_listener = nil
   end
end

function PanicWatcher:_get_panicking_from()
   local threat = stonehearth.combat:get_panicking_from(self._entity)

   if threat and threat:is_valid() then
      return threat
   end
   return nil
end

function PanicWatcher:_on_check_panicking()
   local threat = self:_get_panicking_from()

   if threat then
      self:_set_panicking(threat)
   end
end

function PanicWatcher:_set_panicking(threat)
   self._ai:set_think_output({ threat = threat })
   if self._panicking_listener then
      self._panicking_listener:destroy()
      self._panicking_listener = nil
   end
end

function PanicWatcher:destroy()
   if self._panicking_listener then
      self._panicking_listener:destroy()
      self._panicking_listener = nil
   end
end

return PanicWatcher
