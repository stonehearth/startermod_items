local ItemPathfinder = class()

function ItemPathfinder:__init(entity, solved, filter)
   self._filter = filter
   self._path_finder = native:create_path_finder('items tracker', entity, solved, filter)

   local ec = radiant.entities.get_root_entity():get_component('entity_container'):get_children()

   -- put a trace on the root entity container to detect when items 
   -- go on and off the terrain.  each item is forwarded to the
   -- appropriate tracker.
   self._trace = ec:trace('tracking items on terrain')
   self._trace:on_added(function (id, entity)
         self:_add_entity_to_terrain(id, entity)
      end)
   self._trace:on_removed(function (id)
         self:_remove_entity_from_terrain(id)
      end)

   -- UGGGG. ITERATE THROUGH EVERY ITEM IN THE WORLD. =..(
   for id, item in ec:items() do
      self:_add_entity_to_terrain(id, item)
   end
end

function ItemPathfinder:set_solved_cb(cb)
   self._path_finder:set_solved_cb(cb)
end

function ItemPathfinder:remove_item(entity)
   self._path_finder:remove_destination(entity)
end

function ItemPathfinder:stop()
   if self._path_finder then
      self._path_finder:stop()
      self._path_finder = nil
   end
   if self._trace then
      self._trace:destroy()
      self._trace = nil
   end
end

function ItemPathfinder:_add_entity_to_terrain(id, entity)
   if entity then   
      local item = entity:get_component('item')
      if item then
         if self._filter(entity) then
            self._path_finder:add_destination(entity)
         end
      end
   end
end

function ItemPathfinder:_remove_entity_from_terrain(id)
   self._path_finder:remove_destination(id)
end

function ItemPathfinder:find_path_to_item(source_entity, cb, filter)
   local solved = function (path)
      cb(path)
   end
   self._path_finder:add_entity(source_entity, solved, filter)
end

return ItemPathfinder
