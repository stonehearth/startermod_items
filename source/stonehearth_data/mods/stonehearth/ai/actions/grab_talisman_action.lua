local personality_service = require 'services.personality.personality_service'
local Entity = _radiant.om.Entity

local GrabTalismanAction = class()
GrabTalismanAction.name = 'grab talisman'
GrabTalismanAction.does = 'stonehearth:grab_promotion_talisman'
GrabTalismanAction.args = {
   talisman = Entity,
   workshop = Entity,
   trigger_fn = 'function'
}
GrabTalismanAction.version = 2
GrabTalismanAction.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(GrabTalismanAction)
            :execute('stonehearth:drop_carrying')
            :execute('stonehearth:goto_entity', { entity = ai.ARGS.workshop })
            :execute('stonehearth:remove_child_entity', { container = ai.ARGS.workshop, child = ai.ARGS.talisman })
            :execute('stonehearth:run_effect', {
               effect = 'promote',
               trigger_fn = ai.ARGS.trigger_fn,
               args = { talisman = ai.ARGS.talisman }
              })
