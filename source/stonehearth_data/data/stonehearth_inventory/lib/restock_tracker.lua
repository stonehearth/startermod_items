local RestockTracker = class()

function RestockTracker:__init()
   self._pathfinder = native:create_multi_path_finder('restock tracker')
   self._restock_paths = {}
end

function RestockTracker:register_restock_path(item_entity, path)
   assert(tree)

   self._restock_paths[item_entity:get_id()] = path
   self._pathfinder:add_destination(item_entity)
end

function RestockTracker:find_restock_paths(worker_entity, cb)
   local solved = function (path)
      self._pathfinder:remove_entity(worker_entity:get_id())

      local item_entity = path:get_entity()
      local restock_path = self._restock_paths[item_entity:get_id()]
      if restock_path then
         cb(item_entity, path, nil, restock_path)
      end
   end

   self._pathfinder:add_entity(worker_entity, solved, nil)
end

return RestockTracker
