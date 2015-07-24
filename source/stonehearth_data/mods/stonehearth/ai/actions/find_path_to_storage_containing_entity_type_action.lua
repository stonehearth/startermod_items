local Path = _radiant.sim.Path
local Entity = _radiant.om.Entity
local FindPathToStorageContainingEntityType = class()
local constants = require 'constants'

FindPathToStorageContainingEntityType.name = 'find path to entity type'
FindPathToStorageContainingEntityType.does = 'stonehearth:find_path_to_storage_containing_entity_type'
FindPathToStorageContainingEntityType.args = {
   filter_fn = 'function',             -- entity to find a path to
   description = 'string',             -- description of the initiating compound task (for debugging)
   range = {
      default = 128,
      type = 'number',
   },
}
FindPathToStorageContainingEntityType.think_output = {
   destination = Entity,   -- the destination (container)
   path = Path,            -- the path to destination, from the current Entity
}
FindPathToStorageContainingEntityType.version = 2
FindPathToStorageContainingEntityType.priority = 1

local ALL_FILTER_FNS = {}

function FindPathToStorageContainingEntityType:start_thinking(ai, entity, args)
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

   local filter_fn = ALL_FILTER_FNS[args.filter_fn]

   if not filter_fn then
      filter_fn = function(entity)
         local storage = entity:get_component('stonehearth:storage')

         if not storage then
            return false
         end

         -- Don't take items out of non-public storage (e.g.hearthling backpacks)
         if not storage:is_public() then
            return false
         end

         for _, item in pairs(storage:get_items()) do
            if args.filter_fn(item) then
               return true
            end
         end

         return false
      end
      ALL_FILTER_FNS[args.filter_fn] = filter_fn
   end

   self._location = ai.CURRENT.location
   self._log:info('creating bfs pathfinder for %s (container) @ %s', self._description, self._location)

   -- many actions in our dispatch tree may be asking to find paths to items of
   -- identical types.  for example, each structure in an all wooden building will
   -- be asking for 'wood resource' materials.  rather than start one bfs pathfinder
   -- for each action in the tree, they'll all share the one managed by the
   -- 'stonehearth:pathfinder' component.  this is a massive performance boost.
   self._pathfinder = entity:add_component('stonehearth:pathfinder')
                                 :find_path_to_entity_type(ai.CURRENT.location, -- where to search from?
                                                           filter_fn,      -- the actual filter function
                                                           self._description,    -- for those of us in meat space
                                                           solved)              -- our solved callback
end

function FindPathToStorageContainingEntityType:stop_thinking(ai, entity, args)
   self:_destroy_pathfinder('stop_thinking')
end

function FindPathToStorageContainingEntityType:_destroy_pathfinder(reason)
   if self._pathfinder then
      self._pathfinder:destroy()
      self._pathfinder = nil
      self._log:info('destroying bfs pathfinder for %s @ %s (reason:%s)', self._description, self._location, reason)
   end
end

return FindPathToStorageContainingEntityType
