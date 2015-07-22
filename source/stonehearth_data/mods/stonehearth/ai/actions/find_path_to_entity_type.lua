local Path = _radiant.sim.Path
local Entity = _radiant.om.Entity
local FindPathToEntityType = class()

FindPathToEntityType.name = 'find path to entity type'
FindPathToEntityType.does = 'stonehearth:find_path_to_entity_type'
FindPathToEntityType.args = {
   filter_fn = 'function',             -- entity to find a path to
   description = 'string',             -- description of the initiating compound task (for debugging)
   range = {
      default = 128,
      type = 'number',
   },
}
FindPathToEntityType.think_output = {
   destination = Entity,   -- the destination
   path = Path,            -- the path to destination, from the current Entity
}
FindPathToEntityType.version = 2
FindPathToEntityType.priority = 1

function FindPathToEntityType:start_thinking(ai, entity, args)
   self._description = args.description
   self._log = ai:get_log()

   local solved = function(path)
      local destination = path:get_destination()
      self:_destroy_pathfinder('solution is' .. tostring(destination))
      ai:set_think_output({
            path = path,
            destination = destination,
         })
   end

   self._location = ai.CURRENT.location
   self._log:info('creating bfs pathfinder for %s @ %s', self._description, self._location)

   -- many actions in our dispatch tree may be asking to find paths to items of
   -- identical types.  for example, each structure in an all wooden building will
   -- be asking for 'wood resource' materials.  rather than start one bfs pathfinder
   -- for each action in the tree, they'll all share the one managed by the
   -- 'stonehearth:pathfinder' component.  this is a massive performance boost.
   self._pathfinder = entity:add_component('stonehearth:pathfinder')
                                 :find_path_to_entity_type(ai.CURRENT.location, -- where to search from?
                                                           args.filter_fn,      -- the actual filter function
                                                           self._description,    -- for those of us in meat space
                                                           solved)              -- our solved callback
   ai:set_debug_pathfinder_metadata(self._pathfinder:get_pathfinder_metadata())
end

function FindPathToEntityType:stop_thinking(ai, entity, args)
   self:_destroy_pathfinder('stop_thinking')
end

function FindPathToEntityType:_destroy_pathfinder(reason)
   if self._pathfinder then
      local count = self._pathfinder:destroy()
      self._pathfinder = nil
      self._log:info('destroying bfs pathfinder for %s @ %s (%d remaining, reason:%s)', self._description, self._location, count, reason)
   end
end

return FindPathToEntityType
