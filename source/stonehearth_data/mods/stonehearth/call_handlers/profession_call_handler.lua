local GrabTalismanAction = require 'ai.actions.grab_talisman_action'
local ProfessionCallHandler = class()

-- server side object to handle creation of the workbench.  this is called
-- by doing a POST to the route for this file specified in the manifest.

function ProfessionCallHandler:grab_promotion_talisman(session, response, person, talisman)
   radiant.events.broadcast_msg(
      'stonehearth:events:compulsion_event',
      GrabTalismanAction,
      person,
      {talisman = talisman})
      return true
end

return ProfessionCallHandler