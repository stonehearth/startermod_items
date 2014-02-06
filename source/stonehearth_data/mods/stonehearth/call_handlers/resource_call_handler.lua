local priorities = require('constants').priorities.worker_task

local ResourceCallHandler = class()

--- Remote entry point for requesting that a tree get harvested
-- @param tree The entity which you would like chopped down
-- @return true on success, false on failure
function ResourceCallHandler:harvest_tree(session, response, tree)
   local town = stonehearth.town:get_town(session.faction)
   return town:harvest_resource_node(tree)
end

function ResourceCallHandler:harvest_plant(session, response, plant)
   local town = stonehearth.town:get_town(session.faction)
   return town:harvest_renewable_resource_node(tree)
end

function ResourceCallHandler:shear_sheep(session, response, sheep)
   asset(false, 'shear_sheep was broken, so i deleted it =)  -tony')
end

return ResourceCallHandler
