local Path = _radiant.sim.Path
local Entity = _radiant.om.Entity
local FindPathToEntityType = class()

FindPathToEntityType.name = 'find path to entity type'
FindPathToEntityType.does = 'stonehearth:find_path_to_entity_type'
FindPathToEntityType.args = {
   filter_fn = 'function',   -- entity to find a path to
}
FindPathToEntityType.think_output = {
   destination = Entity,   -- the destination
   path = Path,            -- the path to destination, from the current Entity
}
FindPathToEntityType.version = 2
FindPathToEntityType.priority = 1

local log = radiant.log.create_logger('ai.find_path_to_entity')

function FindPathToEntityType:start_thinking(ai, entity, args)
   local filter_fn = args.filter_fn

   local on_added = function(id, entity)
      if filter_fn(entity) then
         self._pathfinder:add_destination(entity)
      end
   end
   local on_removed = function(id)
      self._pathfinder:remove_destination(id)
   end
   local solved = function(path)
      self._pathfinder:stop()
      self._pathfinder = nil
      ai:set_think_output({path = path, destination = path:get_destination()})
   end
   self._pathfinder = _radiant.sim.create_path_finder(entity, 'goto entity action')
                         :set_source(ai.CURRENT.location)
                         :set_solved_cb(solved)
   self._promise = radiant.terrain.trace_world_entities('find path to entity type', on_added, on_removed)
end

function FindPathToEntityType:stop_thinking(ai, entity)
   if self._pathfinder then
      self._pathfinder:stop()
      self._pathfinder = nil
   end
   if self._promise then
      self._promise:destroy()
      self._promise = nil
   end
end

function FindPathToEntityType:run(ai, entity)
   -- nothing to do...
end

function FindPathToEntityType:stop()
   if self._pathfinder then
      self._pathfinder:stop()
      self._pathfinder = nil
   end
   if self._promise then
      self._promise:destroy()
      self._promise = nil
   end
end

return FindPathToEntityType
