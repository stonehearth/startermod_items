local GrabTalismanAction = require 'stonehearth_classes/ai/actions/grab_talisman_action'
local PromoteCommand = class()

-- server side object to handle creation of the workbench.  this is called
-- by doing a POST to the route for this file specified in the manifest.

function PromoteCommand:handle_request(session, postdata)
   local target_talisman = radiant.entities.get_entity(postdata.data.talisman)
   radiant.events.broadcast_msg(
      'stonehearth.events.compulsion_event',
      GrabTalismanAction,
      radiant.entities.get_entity(postdata.targetPerson),
      {talisman = target_talisman})
   --TODO: do we want to return anything?
end

return PromoteCommand