local constants = require 'constants'

local PanicDispatcher = class()
PanicDispatcher.name = 'panic dispatcher'
PanicDispatcher.status_text_key = 'ai_status_text_panick'
PanicDispatcher.does = 'stonehearth:combat'
PanicDispatcher.args = {}
PanicDispatcher.version = 2
PanicDispatcher.priority = constants.priorities.combat.PANIC

local ai = stonehearth.ai
return ai:create_compound_action(PanicDispatcher)
         :execute('stonehearth:combat:panic_watcher')
         :execute('stonehearth:set_posture', { posture = 'stonehearth:panic' })
         :execute('stonehearth:combat:panic', { threat = ai.BACK(2).threat })
         :execute('stonehearth:trigger_event', {
            source = ai.ENTITY, 
            event_name = 'stonehearth:panicking', 
            event_args = { threat = ai.BACK(3).threat}
            })
         :execute('stonehearth:combat:stop_panicking')
