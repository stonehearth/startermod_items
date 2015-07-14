local Entity = _radiant.om.Entity

local GrabTalismanTypeAction = class()
GrabTalismanTypeAction.name = 'grab talisman type'
GrabTalismanTypeAction.does = 'stonehearth:grab_promotion_talisman_type'
GrabTalismanTypeAction.status_text_key = 'ai_status_text_promote'

GrabTalismanTypeAction.args = {
   talisman_uri = 'string',
   trigger_fn = 'function',
}
GrabTalismanTypeAction.version = 2
GrabTalismanTypeAction.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(GrabTalismanTypeAction)
            :execute('stonehearth:drop_carrying_now')
            :execute('stonehearth:pickup_item_with_uri', {
                  uri = ai.ARGS.talisman_uri
               })
            :execute('stonehearth:run_effect', {
               effect = 'promote',
               trigger_fn = ai.ARGS.trigger_fn,
               args = { talisman = ai.PREV.item }
              })
