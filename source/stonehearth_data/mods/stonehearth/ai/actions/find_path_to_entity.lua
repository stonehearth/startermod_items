local OptimizedPathfinder = require 'ai.actions.optimized_pathfinder'
local Path = _radiant.sim.Path
local Entity = _radiant.om.Entity

local FindPathToEntity = class()

FindPathToEntity.name = 'find path to entity'
FindPathToEntity.does = 'stonehearth:find_path_to_entity'
FindPathToEntity.args = {
   destination = Entity,      -- entity to find a path to
}
FindPathToEntity.think_output = {
   path = Path,       -- the path to destination, from the current Entity
}
FindPathToEntity.version = 2
FindPathToEntity.priority = 1

function FindPathToEntity:start_thinking(ai, entity, args)
   local on_success = function (path)
      ai:set_think_output({ path = path })
   end

   local on_failure = function (message)
      ai:abort(message)
   end

   self._pathfinder = OptimizedPathfinder(entity, args.destination, on_success, on_failure)
                        :set_start_location(ai.CURRENT.location)
                        :start()
end

function FindPathToEntity:stop_thinking(ai, entity, args)
   if self._pathfinder then
      self._pathfinder:stop()
      self._pathfinder = nil
   end
end

return FindPathToEntity
