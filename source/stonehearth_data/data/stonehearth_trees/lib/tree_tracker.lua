local TreeTracker = class()

function TreeTracker:__init(faction)
   self._pathfinder = _radiant.sim.create_multi_path_finder('tree tracker')
end

function TreeTracker:harvest_tree(tree)
   assert(tree)

   radiant.log.info('harvesting resource entity %d', tree:get_id())
   self._pathfinder:add_destination(tree)
end

function TreeTracker:find_path_to_tree(entity, cb)
   local solved = function (path)
      self._pathfinder:remove_entity(entity:get_id())
      cb(path)
   end
   self._pathfinder:add_entity(entity, solved, nil)
end

function TreeTracker:stop_search(entity)
   if entity then
      self._pathfinder:remove_entity(entity:get_id())
   end
end

return TreeTracker
