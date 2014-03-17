local priorities = require('constants').priorities.worker_task
local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3

local UnloadInventoryCallHandler = class()

function UnloadInventoryCallHandler:unload_inventory(session, response, entity)
   if not entity:get_component('stonehearth:ai') then
      return
   end
   
   local town = stonehearth.town:get_town(session.player_id)
   town:command_unit(entity, 'stonehearth:unit_control:unload_inventory')
            :once()
            :start()
   return true
end

return UnloadInventoryCallHandler
