local constants = require 'constants'
local GrabTalismanAction = require 'ai.actions.grab_talisman_action'
local ProfessionCallHandler = class()

-- server side object to handle creation of the workbench.  this is called
-- by doing a POST to the route for this file specified in the manifest.

function ProfessionCallHandler:grab_promotion_talisman(session, response, person, talisman)
   local town = stonehearth.town:get_town(session.player_id)
   town:promote_citizen(person, talisman)
   return true
end

return ProfessionCallHandler