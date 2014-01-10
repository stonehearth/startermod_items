local Path = _radiant.sim.Path
local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3
local FindPathToEntity = class()

FindPathToEntity.name = 'find path to entity'
FindPathToEntity.does = 'stonehearth:find_path_to_entity'
FindPathToEntity.args = {
   finish = Entity,      -- entity to find a path to
   reserve_region = {    -- reserve the destination region?
      type = 'boolean',
      default = false,
   }
}
FindPathToEntity.think_output = {
   path = Path,       -- the path to destination, from the current Entity
}
FindPathToEntity.version = 2
FindPathToEntity.priority = 1

local log = radiant.log.create_logger('ai.find_path_to_entity')

function FindPathToEntity:start_thinking(ai, entity, args)
   local destination = args.finish
   
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
      ai:set_think_output({path = path})
   end
   log:spam('finding path from CURRENT.location %s to %s', tostring(ai.CURRENT.location), tostring(destination))
   self._pathfinder = _radiant.sim.create_path_finder(entity, 'find path to entity')
                         :set_source(ai.CURRENT.location)
                         :add_destination(destination)
                         :set_solved_cb(solved)
end

function FindPathToEntity:stop_thinking(ai, entity)
   if self._pathfinder then
      self._pathfinder:stop()
      self._pathfinder = nil
   end
   if self._trace then
      self._trace:destroy()
      self._trace = nil
   end
end

function FindPathToEntity:run(ai, entity)
   -- nothing to do...
end

function FindPathToEntity:stop()
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
