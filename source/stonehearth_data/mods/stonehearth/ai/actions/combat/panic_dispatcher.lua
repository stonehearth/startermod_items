local constants = require 'constants'
local Entity = _radiant.om.Entity

local PanicDispatcher = class()
PanicDispatcher.name = 'panic dispatcher'
PanicDispatcher.status_text = 'panicking!'
PanicDispatcher.does = 'stonehearth:combat'
PanicDispatcher.args = {
   enemy = Entity,
}
PanicDispatcher.version = 2
PanicDispatcher.priority = constants.priorities.combat.PANIC

local ai = stonehearth.ai
return ai:create_compound_action(PanicDispatcher)
         :execute('stonehearth:combat:panic_watcher')
         :execute('stonehearth:set_posture', { posture = 'stonehearth:panic' })
         :execute('stonehearth:combat:panic', { enemy = ai.ARGS.enemy })
         :execute('stonehearth:unset_posture', { posture = 'stonehearth:panic' })
         :execute('stonehearth:combat:stop_panicking')
