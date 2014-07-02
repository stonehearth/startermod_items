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
   local threat = stonehearth.combat:get_panicking_from(entity)

   if threat and threat:is_valid() then
      ai:set_think_output({ threat = threat })
   end
end

return PanicWatcher
