local GrabTalismanAction = radiant.mods.require('stonehearth.lib.ai.actions.grab_talisman_action')
local JobsCallHandler = class()

-- server side object to handle creation of the workbench.  this is called
-- by doing a POST to the route for this file specified in the manifest.

function JobsCallHandler:grab_promotion_talisman(session, response, person, talisman)
   radiant.events.broadcast_msg(
      'stonehearth.events.compulsion_event',
      GrabTalismanAction,
      person,
      {talisman = talisman})
      return true
end

return JobsCallHandler