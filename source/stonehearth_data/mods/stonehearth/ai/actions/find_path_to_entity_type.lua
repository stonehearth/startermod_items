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
      self:_destroy_pathfinder()
      ai:set_think_output({
            path = path,
            destination = path:get_destination(),
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
                                 :find_path_to_item_type(ai.CURRENT.location, -- where to search from?
                                                         args.filter_fn,      -- the actual filter function
                                                         self._description,    -- for those of us in meat space
                                                         solved)              -- our solved callback
end

function FindPathToEntityType:stop_thinking(ai, entity, args)
   self:_destroy_pathfinder()
end

function FindPathToEntityType:_destroy_pathfinder()
   if self._pathfinder then
      local count = self._pathfinder:destroy()
      self._pathfinder = nil
      self._log:info('destroying bfs pathfinder for %s @ %s (%d remaining)', self._description, self._location, count)
   end
end

return FindPathToEntityType
