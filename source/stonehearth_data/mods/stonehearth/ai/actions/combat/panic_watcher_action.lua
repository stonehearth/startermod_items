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

function PanicWatcher:start_thinking(ai, entity, args)
   self._entity = entity
   self._ai = ai

   if not self:_check_panicking() then
      self._listening = true
      radiant.events.listen(self._entity, 'stonehearth:panic', self, self._check_panicking)
   end
end

function PanicWatcher:stop_thinking(ai, entity, args)
   if self._listening then
      radiant.events.unlisten(self._entity, 'stonehearth:panic', self, self._check_panicking)
      self._listening = false
   end
end

function PanicWatcher:_check_panicking()
   local threat = stonehearth.combat:get_panicking_from(self._entity)

   if threat and threat:is_valid() then
      self._ai:set_think_output({ threat = threat })
      self._listening = false
      return radiant.events.UNLISTEN
   end
   return nil
end

return PanicWatcher
