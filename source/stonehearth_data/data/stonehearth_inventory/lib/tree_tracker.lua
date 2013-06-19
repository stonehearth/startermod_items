local TreeTracker = class()

function TreeTracker:__init(faction)
   self._pathfinder = native:create_multi_path_finder('tree tracker')
   self._callbacks = {}
   radiant.events.listen('radiant.events.gameloop', self)
end

TreeTracker['radiant.events.gameloop'] = function(self)
   local path = self._pathfinder:get_solution()
   if path then
      local entity_id = path:get_entity():get_id()
      local cb = self._callbacks(entity_id)
      if cb then
         cb(path)
      end
   end   
end

function TreeTracker:harvest_tree(tree)
   assert(tree)

   radiant.log.info('harvesting resource entity %d', tree:get_id())
   local destination = tree:get_component('destination')
   if destination then
      self._pathfinder:add_destination(destination)
   end
end

function TreeTracker:find_path_to_tree(entity, cb)
   self._pathfinder:add_entity(entity)
   self._callbacks[entity:get_id()] = cb
end

return TreeTracker
