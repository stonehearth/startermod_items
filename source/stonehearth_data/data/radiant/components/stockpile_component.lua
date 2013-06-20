local StockpileComponent = class()

function StockpileComponent:__init(entity)
   self._entity = entity
   self._destination = entity:add_component('destination')
   self._data = {
      items = {},
      size  = { 0, 0 }
   }
end

function StockpileComponent:extend(json)
   if json.size then
      self:set_size(json.size)
   end
   -- not so much...
   if not self._ec_trace then
      local root = radiant.entities.get_root_entity()
      local ec = radiant.entities.get_root_entity():get_component('entity_container')
      local children = ec:get_children()
      
      self._ec_trace = children:trace('tracking stockpile')
      self._ec_trace:on_added(function (id, entity)
            self:_add_item(entity)
         end)
      self._ec_trace:on_removed(function (id, entity)
            self:_remove_item(id)
         end)
      
      local boxed_transform = self._entity:add_component('mob'):get_boxed_transform()
      self._mob_trace = boxed_transform:trace('stockpile tracking self position')
      self._mob_trace:on_changed(function()
            self:_rebuild_item_data()
         end)

      self:_rebuild_item_data()      
   end
end

function StockpileComponent:is_full()
   return false
end

function StockpileComponent:get_size()
   return { self._data.size[1], self._data.size[2] }
end

function StockpileComponent:set_size(size)
   self._data.size = { size[1], size[2] }  
   radiant.components.mark_dirty(self, self._data)
end

function StockpileComponent:_add_item(entity)
   local size = self:get_size()
   local origin = Point3(radiant.entities.get_world_grid_location(entity))
   local bounds = Cube3(origin, Point3(origin.x + size[1], origin.y, origin.z + size[2]))
   
   if bounds:contains(origin) then
      local item = entity:get_component('item')
      if item then
         -- hold onto the item...
         self._data.items[entity:get_id()] = item
         radiant.components.mark_dirty(self, self._data)

         -- update our destination component to *remove* the space
         -- where the item is, since we can't drop anything there
         local region = self._destination:get_region()
         local offset = origin - radiant.entities.get_world_grid_location(self._entity)
         region:remove_point(offset)
      end
   end
end

function StockpileComponent:_remove_item(id)
   local entity = self._data.items[id]
   if entity then
      self._data.items[id] = nil
      radiant.components.mark_dirty(self, self._data)

      -- add this point to our destination region
      local region = self._destination:get_region()
      local origin = radiant.entities.get_world_grid_location(entity)
      local offset = origin - radiant.entities.get_world_grid_location(self._entity)
      region:add_point(offset)
   end
end

function StockpileComponent:_mark_dirty()
   if not self._dirty then
      self._dirty = true
      events.notify_game_loop_stage('post_trace_firing', function()
         self._update_state()
         self._dirty = false
      end)      
   end
end

function StockpileComponent:_rebuild_item_data()
   local size = self:get_size()
   local bounds = Cube3(Point3(0, 0, 0), Point3(size[1], 1, size[2]))
   local region = Region3(bounds)
   self._destination:set_region(region)

   self._data.items = {}
   radiant.components.mark_dirty(self, self._data)

   local ec = radiant.entities.get_root_entity()
                  :get_component('entity_container')
                  :get_children()

   for id, child in ec:items() do
      self:_add_item(child)
   end
end


return StockpileComponent
