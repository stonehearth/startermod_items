local priorities = require('constants').priorities.worker_task
local inventory_service = stonehearth.inventory

local StockpileComponent = class()
StockpileComponent.__classname = 'StockpileComponent'
local log = radiant.log.create_logger('stockpile')

local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3

local all_stockpiles = {}


--- Returns the stockpile which contains an item
-- This implementation is super slow, iterating through every stockpile in
-- the world (eek!).  A better idea would be to add a 'owning_stockpile'
-- component or something with a back pointer to the appropriate stockpile.
-- @param entity The entity allegedly in a stockpile
-- @returns The stockpile containing entity, or nil

function get_stockpile_containing_entity(entity)
   local location = radiant.entities.get_world_grid_location(entity)
   for id, stockpile in pairs(all_stockpiles) do
      log:spam('checking %s pos:%s inside a stockpile', entity, location)
      if not stockpile then
         log:spam('  nil stockpile!  nope!')
      else
         local name = tostring(stockpile:get_entity())
         local bounds = stockpile:get_bounds();
         if not bounds:contains(location) then
            log:spam('  %s -> nope! wrong bounds, %s', name, bounds)
         elseif not stockpile:can_stock_entity(entity) then
            log:spam('  %s -> nope! cannot stock', name)
         else
            log:spam('  %s -> yup!', name)
            return stockpile
         end
      end        
   end
end

function StockpileComponent:__init(entity, data_binding)
   self._entity = entity
   self._filter = nil

   self._destination = entity:add_component('destination')
   self._data = {
      stocked_items = {},
      item_locations = {},
      size  = { 0, 0 },
      filter = self._filter
   }
   self._data_binding = data_binding
   self._data_binding:update(self._data)

   self._destination:set_region(_radiant.sim.alloc_region())
                    :set_reserved(_radiant.sim.alloc_region())
                    :set_auto_update_adjacent(true)

   radiant.events.listen(radiant.events, 'stonehearth:gameloop', self, self.on_gameloop)
   all_stockpiles[self._entity:get_id()] = self
end

function StockpileComponent:get_entity()
   return self._entity
end

function StockpileComponent:destroy()
   log:info('%s destroying stockpile component', self._entity)
   all_stockpiles[self._entity:get_id()] = nil
   for id, item in pairs(self._data.stocked_items) do
      self:_remove_item_from_stock(id)
   end
   if self._ec_trace then
      self._ec_trace:destroy()
      self._ec_trace = nil
   end
   if self._task then
      self._task:destroy()
      self._task = nil
   end
   if self._ec_trace then
      self._ec_trace:destroy()
      self._ec_trace = nil
   end
   if self._unit_info_trace then
      self._unit_info_trace:destroy()
      self._unit_info_trace = nil
   end
   if self._mob_trace then
      self._mob_trace:destroy()
      self._mob_trace= nil
   end
end

function StockpileComponent:set_filter(filter)
   self._filter = filter
   self._data.filter = self._filter
   self._data_binding:update(self._data)

   -- for items that no longer match the filter, 
   -- remove then re-add the item from its parent. Other stockpiles
   -- will be notified when it is added to the world and will
   -- reconsider it for placement.
   for id, location in pairs(self._data.item_locations) do
      local item = radiant.entities.get_entity(id)
      local can_stock = self:can_stock_entity(item)
      local is_stocked = self._data.stocked_items[id] ~= nil
      if can_stock and not is_stocked then
         self:_add_item_to_stock(item)
      end
      if not can_stock and is_stocked then
         self:_remove_item_from_stock(item:get_id())
      end
   end

   self:_create_worker_tasks()
   return self
end

-- xxx: the 'fire one when i'm constructed' pattern again...
function StockpileComponent:on_gameloop()
   radiant.events.unlisten(radiant.events, 'stonehearth:gameloop', self, self.on_gameloop)
   local root = radiant.entities.get_root_entity()
   local ec = radiant.entities.get_root_entity():get_component('entity_container')

   self._ec_trace = ec:trace_children('tracking stockpile')
                           :on_added(function (id, entity)
                              self:_add_item(entity)
                           end)
                           :on_removed(function (id, entity)
                              self:_remove_item(id)
                           end)

   local mob = self._entity:add_component('mob')
   self._mob_trace = mob:trace_transform('stockpile tracking self position')
                        :on_changed(function()
                              self:_rebuild_item_data()
                           end)

   local unit_info = self._entity:add_component('unit_info')
   self._unit_info_trace = unit_info:trace_faction('stockpile tracking faction')
                                       :on_changed(function()
                                             self:_assign_to_faction()
                                          end)
      
   self:_assign_to_faction()
   self:_rebuild_item_data()
end

function StockpileComponent:extend(json)
   if json.size then
      self:set_size(json.size)
   end
end

function StockpileComponent:get_items()
   return self._data.stocked_items;
end

function StockpileComponent:set_outbox(value)
   self._is_outbox = value
end

function StockpileComponent:is_outbox(value)
   return self._is_outbox
end

function StockpileComponent:_get_bounds()
   local size = self:get_size()
   local bounds = Cube3(Point3(0, 0, 0), Point3(size[1], 1, size[2]))
   return bounds
end

function StockpileComponent:get_bounds()
   local size = self:get_size()
   local origin = radiant.entities.get_world_grid_location(self._entity)
   local bounds = Cube3(origin, Point3(origin.x + size[1], origin.y + 1, origin.z + size[2]))
   return bounds
end

function StockpileComponent:is_full()
   if self._destination and self._destination:is_valid() then
      return self._destination:get_region():get():empty()
   end
   return true
end

function StockpileComponent:bounds_contain(item_entity)
   local location = radiant.entities.get_world_grid_location(item_entity)
   local world_bounds = self:get_bounds()
   return world_bounds:contains(location)
end

function StockpileComponent:get_size()
   return { self._data.size[1], self._data.size[2] }
end

function StockpileComponent:set_size(size)
   self._data.size = { size[1], size[2] }
   self:_rebuild_item_data()
end

function StockpileComponent:_add_to_region(entity)
   local location = radiant.entities.get_world_grid_location(entity)
   local pt = location - radiant.entities.get_world_grid_location(self._entity)
   log:debug('adding point %s to region', tostring(pt))
   self._data.item_locations[entity:get_id()] = pt
   
   local was_full = self:is_full()
   self._destination:get_region():modify(function(cursor)
      cursor:subtract_point(pt)
   end)
   if not was_full and self:is_full() then
      radiant.events.trigger(self, 'space_available', self, false)
   end
   
   log:debug('finished adding point %s to region', tostring(pt))
   self:_unreserve(location)
end

function StockpileComponent:_remove_from_region(id)
   local pt = self._data.item_locations[id]
   log:debug('removing point %s from region', tostring(pt))
   self._data.item_locations[id] = nil
   
   local was_full = self:is_full()
   self._destination:get_region():modify(function(cursor)
      cursor:add_point(pt)
   end)
   log:debug('finished removing point %s from region', tostring(pt))
   
   if was_full and not self:is_full() then
      radiant.events.trigger(self, 'space_available', self, true)
   end
end

function StockpileComponent:_reserve(location)
   local pt = location - radiant.entities.get_world_grid_location(self._entity)
   log:debug('adding point %s to reserve region', tostring(pt))
   self._destination:get_reserved():modify(function(cursor)
      cursor:add_point(pt)
   end)
   log:debug('finished adding point %s to reserve region', tostring(pt))
end

function StockpileComponent:_unreserve(location)
   local pt = location - radiant.entities.get_world_grid_location(self._entity)
   log:debug('removing point %s from reserve region', tostring(pt))
   self._destination:get_reserved():modify(function(cursor)
      cursor:subtract_point(pt)
   end)
   log:debug('finished removing point %s from reserve region', tostring(pt))
end

function StockpileComponent:_add_item(entity)
   if self:bounds_contain(entity) and entity:get_component('item') then
      -- whether we can stock the object or not, take this space out of
      -- our destination region.
      self:_add_to_region(entity)
     
      if self:can_stock_entity(entity) then
         log:debug('adding %s to stock', entity)
         self:_add_item_to_stock(entity)
      end
   end
end

function StockpileComponent:_add_item_to_stock(entity)
   assert(self:can_stock_entity(entity) and self:bounds_contain(entity))
   
   -- hold onto the item...
   self._data.stocked_items[entity:get_id()] = entity
   self._data_binding:mark_changed()

   -- add the item to the inventory 
   radiant.events.trigger(self._entity, "stonehearth:item_added", { 
      storage = self._entity,
      item = entity 
   })
end

function StockpileComponent:_remove_item(id)
   -- remove from the region
   if self._data.item_locations[id] then
      self:_remove_from_region(id)
      if self._data.stocked_items[id] then
         self:_remove_item_from_stock(id)
      end
   end
end

function StockpileComponent:_remove_item_from_stock(id)
   assert(self._data.stocked_items[id])
   
   local entity = self._data.stocked_items[id]
   self._data.stocked_items[id] = nil
   self._data_binding:mark_changed()

   --Remove items that have been taken out of the stockpile
   if entity and entity:is_valid() then
      radiant.events.trigger(self._entity, "stonehearth:item_removed", { 
         storage = self._entity,
         item = entity
      })
      radiant.events.trigger(stonehearth.ai, 'stonehearth:reconsider_stockpile_item', entity)
   end
end

function StockpileComponent:_rebuild_item_data()
   self._destination:get_region():modify(function(cursor)
      cursor:clear()
      cursor:add_cube(self:_get_bounds())
   end)
   self._destination:get_reserved():modify(function(cursor)
      cursor:clear()
   end)

   -- xxx: if this ever gets called with stocked items, we're probably
   -- going to screw up the inventory system here (they get orphended!)
   
   self._data.stocked_items = {}
   self._data.item_locations = {}
   self._data_binding:mark_changed()

   local ec = radiant.entities.get_root_entity()
                  :get_component('entity_container')

   for id, child in ec:each_child() do
      if child and child:is_valid() then  -- xxx: why is this necessary?
         self:_add_item(child)
      end
   end
end

function StockpileComponent:_assign_to_faction()
   local faction = self._entity:add_component('unit_info'):get_faction()

   local old_faction = self._faction
   self._faction = faction
   
   if not old_faction or self._faction ~= old_faction then
      if old_faction then
         -- unregister from the current inventory service
         local inventory = inventory_service:get_inventory(old_faction)
         inventory:remove_storage(self._entity)
      end

      -- register with the inventory service for this faction
      if faction then
         local inventory = inventory_service:get_inventory(faction)
         inventory:add_storage(self._entity)
         radiant.events.listen(inventory, 'stonehearth:item_added', self, self._on_item_added_to_inventory)
         radiant.events.listen(inventory, 'stonehearth:item_removed', self, self._on_item_removed_from_inventory)
      end      
      self:_create_worker_tasks()
   end
end

function StockpileComponent:_on_item_added_to_inventory(e)
   -- xxx: need to poke the pathfinder in the restock task somehow... hmm..
end

function StockpileComponent:_on_item_removed_from_inventory(e)
   -- xxx: need to poke the pathfinder in the restock task somehow... hmm..
end

--- Returns whether or not the stockpile should stock this entity
-- @param entity The entity you're interested in
-- @return true if the entity can be stocked here, false otherwise.

function StockpileComponent:can_stock_entity(entity)
   if not entity or not entity:get_component('item') then
      log:spam('nil or non-item entity %s cannot be stocked', entity)
      return false
   end
   
   if not self:_is_in_filter(entity) then
      log:spam('%s not in filter and cannot be stocked', entity)
      return false
   end 
   log:spam('%s ok to stock %s!', self._entity, entity)
   return true   
end

function StockpileComponent:_is_in_filter(entity)
   local material = entity:get_component('stonehearth:material')
   if not material then
      return false
   end

   if not self._filter then 
      return true
   end
   
   local in_filter = false

   if self._filter then 
      for i, filter in ipairs(self._filter) do
         if material:is(filter) then
            in_filter = true
            break
         end
      end
   end

   return in_filter
end


--- Creates worker tasks for keeping the stockpile full
-- We schedule two tasks to try to fill up the stockpile.  The first looks for
-- workers who aren't carrying anything and asks them to pick up any item which
-- belongs in the stockpile and is not in *any* other stockpile.  The second
-- asks workers who are carrying items which belong in our stockpile to drop
-- them inside us.

function StockpileComponent:_create_worker_tasks()
   if self._is_outbox then
      return
   end

   if self._task ~= nil then
      self._task:destroy()
      self._task = nil
   end

   local town = stonehearth.town:get_town(self._faction)
   self._task = town:create_worker_task('stonehearth:restock_stockpile', { stockpile = self })
                                   :set_source(self._entity)
                                   :set_name('restock task')
                                   :start()
end

function StockpileComponent:get_item_filter_fn()
   return function(entity)
      log:spam('%s checking ok to pickup', entity)
      if not self:can_stock_entity(entity) then
         log:spam('%s not stockable.  not picking up', entity)
         return false
      end
      local stockpile = get_stockpile_containing_entity(entity)
      if stockpile and not stockpile:is_outbox() then
         log:spam('%s contained in stockpile.  not picking up', entity)
         return false
      end
      log:spam('ok to pickup %s (mob:%d)!', entity, entity:get_component('mob'):get_id())
      return true
   end
end

return StockpileComponent
