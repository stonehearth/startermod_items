local GrabTalismanAction = require 'stonehearth_classes/ai/actions/grab_talisman_action'

function promote(player, args)
   local target_talisman = radiant.entities.get_entity(args.talisman)
   radiant.events.broadcast_msg(
      'stonehearth.events.compulsion_event',
      GrabTalismanAction,
      15, --TODO: replace with the results of the people picker
      {talisman = target_talisman})
end

return promote
