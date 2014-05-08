local Entity = _radiant.om.Entity

local GrabTalismanTypeAction = class()
GrabTalismanTypeAction.name = 'grab talisman type'
GrabTalismanTypeAction.does = 'stonehearth:grab_promotion_talisman_type'
GrabTalismanTypeAction.args = {
   filter_fn = 'function',
   trigger_fn = 'function'
}
GrabTalismanTypeAction.version = 2
GrabTalismanTypeAction.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(GrabTalismanTypeAction)
            :execute('stonehearth:drop_carrying_now')
            :execute('stonehearth:pickup_item_type', { filter_fn = ai.ARGS.filter_fn, description='grab talisman type' })
            :execute('stonehearth:run_effect', {
               effect = 'promote',
               trigger_fn = ai.ARGS.trigger_fn,
               args = { talisman = ai.PREV.item }
              })
