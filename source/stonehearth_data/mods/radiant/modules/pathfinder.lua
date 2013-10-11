local PathFinderWrapper = require 'modules.pathfinder.pathfinder_wrapper'

local pathfinder = {}
local singleton = {
   pathfinders = {}
}


function pathfinder.__init()
   singleton.promise = radiant.terrain.trace_world_entities('radiant pathfinder module',
                                                            _add_entity_to_terrain,
                                                            _remove_entity_from_terrain)
end

function _add_entity_to_terrain(id, entity)
   for id, pf in pairs(singleton.pathfinders) do
      local pathfinder = pf:lock()
      if pathfinder then
         pathfinder:add_destination(entity)
      else
         singleton.pathfinders[id] = nil
      end
   end
end

function _remove_entity_from_terrain(id)
   for id, pf in pairs(singleton.pathfinders) do
      local pathfinder = pf:lock()
      if pathfinder then
         pathfinder:remove_destination(id)
      else
         singleton.pathfinders[id] = nil
      end
   end
end

function pathfinder.create_path_finder(reason)
   return PathFinderWrapper(_radiant.sim.create_path_finder(reason))
end

function pathfinder._track_world_items(pathfinder)
   singleton.pathfinders[pathfinder:get_id()] = pathfinder:to_weak_ref()

   -- xxx: iterate through every item in a range provided by the client
   for id, entity in radiant.terrain.get_world_entities() do
      --TODO: why is this sometimes nil?
      if entity then
         pathfinder:add_destination(entity)
      end
   end
end

pathfinder.__init()

return pathfinder
