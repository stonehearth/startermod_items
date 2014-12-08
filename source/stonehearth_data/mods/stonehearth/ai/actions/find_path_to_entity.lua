local Path = _radiant.sim.Path
local Entity = _radiant.om.Entity
local log = radiant.log.create_logger('pathfinder')

local FindPathToEntity = class()

FindPathToEntity.name = 'find path to entity'
FindPathToEntity.does = 'stonehearth:find_path_to_entity'
FindPathToEntity.args = {
   destination = Entity,   -- entity to find a path to
}
FindPathToEntity.think_output = {
   path = Path,            -- the path to destination, from the current Entity
}
FindPathToEntity.version = 2
FindPathToEntity.priority = 1

function FindPathToEntity:start_thinking(ai, entity, args)
   local on_success = function (path)
      ai:get_log():info('found solution: %s', path)
      ai:set_think_output({ path = path })
      return true
   end
   local on_exhausted = function()
      ai:get_log():warning('search exhausted!')
   end

   self._pathfinder = entity:add_component('stonehearth:pathfinder')
                                 :find_path_to_entity(ai.CURRENT.location, args.destination, on_success, on_exhausted)
end

function FindPathToEntity:stop_thinking(ai, entity, args)
   if self._pathfinder then
      self._pathfinder:destroy()
      self._pathfinder = nil
   end
end

return FindPathToEntity
