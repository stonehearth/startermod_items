local Path = _radiant.sim.Path
local Entity = _radiant.om.Entity
local FindPathToEntityAction = class()

FindPathToEntityAction.name = 'find path to entity'
FindPathToEntityAction.does = 'stonehearth:find_path_to_entity'
FindPathToEntityAction.args = {
   entity = Entity,      -- entity to find a path to
}
FindPathToEntityAction.run_args = {
   path = Path,       -- the path to destination, from the current Entity
}
FindPathToEntityAction.version = 2
FindPathToEntityAction.priority = 1

local log = radiant.log.create_logger('ai.find_path_to_entity')

function FindPathToEntityAction:start_thinking(ai, entity, args)
   local destination = args.entity
   if not destination or not destination:is_valid() then
      ai:abort('invalid entity reference')
   end
   
   self._trace = radiant.entities.trace_location(destination, 'find path to entity')
      :on_changed(function()
         ai:abort('target destination changed')
      end)
      :on_destroyed(function()
         ai:abort('target destination destroyed')
      end)

   local solved = function(path)
      self._pathfinder:stop()
      self._pathfinder = nil
      ai:set_run_arguments({path = path})
   end
   self._pathfinder = _radiant.sim.create_path_finder(entity, 'goto entity action')
                         :add_destination(destination)
                         :set_solved_cb(solved)
end

function FindPathToEntityAction:stop_thinking(ai, entity)
   if self._pathfinder then
      self._pathfinder:stop()
      self._pathfinder = nil
   end
   if self._trace then
      self._trace:destroy()
      self._trace = nil
   end
end

function FindPathToEntityAction:run(ai, entity)
   -- nothing to do...
end

function FindPathToEntityAction:stop()
   if self._pathfinder then
      self._pathfinder:stop()
      self._pathfinder = nil
   end
   if self._trace then
      self._trace:destroy()
      self._trace = nil
   end
end

return FindPathToEntityAction
