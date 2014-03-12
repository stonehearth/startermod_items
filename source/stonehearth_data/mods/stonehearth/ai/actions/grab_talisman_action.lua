local Entity = _radiant.om.Entity

local GrabTalismanAction = class()
GrabTalismanAction.name = 'grab talisman'
GrabTalismanAction.does = 'stonehearth:grab_promotion_talisman'
GrabTalismanAction.args = {
   talisman = Entity,
   trigger_fn = 'function'
}
GrabTalismanAction.version = 2
GrabTalismanAction.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(GrabTalismanAction)
            :execute('stonehearth:drop_carrying_now')
            :execute('stonehearth:reserve_entity', { entity = ai.ARGS.talisman })
            :execute('stonehearth:goto_entity', { entity = ai.ARGS.talisman  })
            :execute('stonehearth:turn_to_face_entity', { entity = ai.ARGS.talisman  })            
            :execute('stonehearth:run_effect', {
               effect = 'promote',
               trigger_fn = ai.ARGS.trigger_fn,
               args = { talisman = ai.ARGS.talisman }
              })
