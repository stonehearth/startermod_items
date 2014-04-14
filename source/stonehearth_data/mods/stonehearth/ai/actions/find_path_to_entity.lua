local Path = _radiant.sim.Path
local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3
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
   local log = ai:get_log()
   local destination = args.destination
   
   if not destination or not destination:is_valid() then
      log:debug('invalid entity reference.  ignorning')
      return
   end

   if log:is_enabled(radiant.log.DEBUG) then
      local destination_location = radiant.entities.get_world_grid_location(destination)
      log:debug('finding path from CURRENT.location %s to %s (@ %s)',
                tostring(ai.CURRENT.location), tostring(destination), tostring(destination_location))
   end

   local direct_path_finder = _radiant.sim.create_direct_path_finder(entity, destination)
                                 :set_start_location(ai.CURRENT.location)

   local path = direct_path_finder:get_path()
   if path then
      log:detail('found direct path!')
      ai:set_think_output({ path = path })
      return
   end
   log:detail('no direct path found.  starting pathfinder.')
   
   self._trace = radiant.entities.trace_location(destination, 'find path to entity')
      :on_changed(function()
         ai:abort('target destination changed')
      end)
      :on_destroyed(function()
         ai:abort('target destination destroyed')
      end)

   local solved = function(path)
      log:debug('solved! %s', tostring(path))
      self._pathfinder:stop()
      self._pathfinder = nil
      ai:set_think_output({path = path})
   end

   self._pathfinder = _radiant.sim.create_astar_path_finder(entity, 'find path to entity')
                         :set_source(ai.CURRENT.location)
                         :add_destination(destination)
                         :set_solved_cb(solved)
end

function FindPathToEntity:stop_thinking(ai, entity, args)
   if self._pathfinder then
      self._pathfinder:stop()
      self._pathfinder = nil
   end
   if self._trace then
      self._trace:destroy()
      self._trace = nil
   end
end

return FindPathToEntity
