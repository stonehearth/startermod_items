local log = radiant.log.create_logger('combat')

local PanicWatcher = class()

PanicWatcher.name = 'panic watcher'
PanicWatcher.does = 'stonehearth:combat:panic_watcher'
PanicWatcher.args = {}
PanicWatcher.version = 2
PanicWatcher.priority = 1

function PanicWatcher:start_thinking(ai, entity, args)
   if stonehearth.combat:is_panicking(entity) then
      ai:set_think_output()
   end
end

return PanicWatcher
