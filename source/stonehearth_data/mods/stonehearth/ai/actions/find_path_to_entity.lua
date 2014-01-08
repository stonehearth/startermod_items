local Path = _radiant.sim.Path
local Entity = _radiant.om.Entity
local FindPathToEntityAction = class()

FindPathToEntityAction.name = 'find path to entity'
FindPathToEntityAction.does = 'stonehearth:find_path_to_entity'
FindPathToEntityAction.args = {
   Entity,      -- entity to find a path to
}
FindPathToEntityAction.run_args = {
   Path,       -- the path to destination, from the current Entity
}
FindPathToEntityAction.version = 2
FindPathToEntityAction.priority = 1

local log = radiant.log.create_logger('ai.find_path_to_entity')

function FindPathToEntityAction:start_background_processing(ai, entity, target)
   if not target or not target:is_valid() then
      ai:abort('invalid entity reference')
   end
   
   self._trace = radiant.entities.trace_location(target, 'find path to entity')
      :on_changed(function()
         ai:abort('target destination changed')
      end)
      :on_destroyed(function()
         ai:abort('target destination destroyed')
      end)

   local solved = function(path)
      self._pathfinder:stop()
      self._pathfinder = nil
      ai:complete_background_processing(path)
   end
   self._pathfinder = _radiant.sim.create_path_finder(entity, 'goto entity action')
                         :add_destination(target)
                         :set_solved_cb(solved)
end

function FindPathToEntityAction:stop_background_processing(ai, entity, target)
   if self._pathfinder then
      self._pathfinder:stop()
      self._pathfinder = nil
   end
   if self._trace then
      self._trace:destroy()
      self._trace = nil
   end
end

function FindPathToEntityAction:run(ai, entity, path)
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
