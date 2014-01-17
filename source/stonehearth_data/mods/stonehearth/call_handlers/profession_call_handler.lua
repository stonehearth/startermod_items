local GrabTalismanAction = require 'ai.actions.grab_talisman_action'
local ProfessionCallHandler = class()

-- server side object to handle creation of the workbench.  this is called
-- by doing a POST to the route for this file specified in the manifest.

function ProfessionCallHandler:grab_promotion_talisman(session, response, person, entity)
   local talisman
   local workshop_component = entity:get_component("stonehearth:workshop")
   if workshop_component then
      -- a workshop was pased in instead of a talisman. grab the talisman from the workshop
      talisman = workshop_component:get_talisman()
   else
      talisman = entity
   end

   -- todo: stick this in a taskmaster manager somewhere so we can show it (and cancel it!)
   local PromoteTaskMaster = require 'services.tasks.task_masters.promote_with_talisman'   
   PromoteTaskMaster(person, talisman)
   return true
end

return ProfessionCallHandler