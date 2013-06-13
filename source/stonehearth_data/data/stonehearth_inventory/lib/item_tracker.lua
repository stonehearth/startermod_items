local ItemTracker = class()

function ItemTracker:__init(faction)
   self._pathfinder = native:create_multi_path_finder('item tracker')
   self._callbacks = {}
   radiant.events.listen('radiant.events.gameloop', self)
end

ItemTracker['radiant.events.gameloop'] = function(self)
   local path = self._pathfinder:get_solution()
   if path then
      local entity_id = path:get_entity():get_id()
      local cb = self._callbacks(entity_id)
      if cb then
         cb(path)
      end
   end   
end

function ItemTracker:track_item(item)
   assert(item)

   radiant.log.info('tracking item %d', item:get_id())
   local destination = item:get_component('destination')
   if destination then
      self._pathfinder:add_destination(destination)
   end
end

function ItemTracker:find_path_to_item(entity, cb, predicate)
   self._pathfinder:add_entity(entity)
   self._callbacks[entity:get_id()] = cb
end

return ItemTracker
