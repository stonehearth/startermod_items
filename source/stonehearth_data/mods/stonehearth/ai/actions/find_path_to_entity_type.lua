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

function FindPathToEntityType:start_thinking(ai, entity, args)
   self._log = ai:get_log()
   self._filter_fn = args.filter_fn
   self:_start_pathfinder(ai, entity)
end

function FindPathToEntityType:stop_thinking(ai, entity)
   self:_stop_pathfinder(ai, entity)
end

function FindPathToEntityType:stop(ai, entity)
   self:_stop_pathfinder(ai, entity)
end

function FindPathToEntityType:_consider_destination(ai, entity, target)
   if self._solution_path then
      -- hmm.  we already have a solution.  what if this object can provide a better one?
      -- do we really want to invalidate our current solution on the *chance* that it
      -- could be?  let's not.  let's just go with what we've got!
      return
   end

   if self._filter_fn(target) then
      local lease = target:get_component('stonehearth:lease_component')
      if lease and not lease:can_acquire('ai_reservation', entity) then
         self._log:debug('ignoring %s (cannot acquire ai lease)', tostring(target))
         return
      end
      self._log:spam('adding entity %s to pathfinder', tostring(target))
      self._pathfinder:add_destination(target)
   end
end

function FindPathToEntityType:_remove_destination(ai, entity, id)
   if self._solution_entity_id then
      if id == self._solution_entity_id then
         -- rats!  the thing we found earlier is no longer in the world.  revoke our
         -- current solution and restart the pathfinder
         ai:clear_think_output()
         self:_stop_pathfinder(ai, entity)
         self:_start_pathfinder(ai, entity)
      else
         -- a destination was removed, but it's not our current solution.  just
         -- ignore it
      end
      return
   end
   
   self._log:spam('removing entity %d to pathfinder', id)
   self._pathfinder:remove_destination(id)
end

function FindPathToEntityType:_start_pathfinder(ai, entity)   
   local on_added = function(id, target)
      self:_consider_destination(ai, entity, target)
   end   
   local on_removed = function(id)
      self:_remove_destination(ai, entity, id)
   end   
   local solved = function(path)
      self._solution_path = path
      self._solution_entity_id = path:get_destination():get_id()
      self._log:debug('solved!')
      ai:set_think_output({path = path, destination = path:get_destination()})
   end
   
   self._log:spam('finding path from CURRENT.location %s to item type...', tostring(ai.CURRENT.location))
   self._pathfinder = _radiant.sim.create_path_finder(entity, 'goto entity action')
                         :set_source(ai.CURRENT.location)
                         :set_solved_cb(solved)
   self._promise = radiant.terrain.trace_world_entities('find path to entity type', on_added, on_removed)
end

function FindPathToEntityType:_stop_pathfinder(ai, entity)
   self._solution_path = nil
   self._solution_entity_id = nil
   self._log:spam('stop thinking.')
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
