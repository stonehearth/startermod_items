local priorities = require('constants').priorities.worker_task
local inventory_service = radiant.mods.load('stonehearth').inventory

local StockpileComponent = class()

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
   for id, stockpile in pairs(all_stockpiles) do
      if stockpile and stockpile:contains(entity) and stockpile:can_stock_entity(entity) then
         return stockpile
      end
   end
end

function StockpileComponent:__init(entity, data_binding)
   self._entity = entity
   self._filter = {
      wood = true,
      stone = true,
      ore = true,
      animal_part = true,
      portal = true,
      furniture = true,
      defense = true,
      light = true,
      decoration = true,
      refined_cloth = true,
      refined_animal_part = true,
      refined_ore = true,
      melee_weapon = true,
      ranged_weapon = true,
      light_armor = true,
      heavy_armor = true,
      exotic_gear = true,
      fruit = true,
      vegetable = true,
      baked = true,
      meat = true,
      drink = true,
      gold = true,
      gem = true
   }

   self._destination = entity:add_component('destination')
   self._data = {
      items = {},
      size  = { 0, 0 },
      filter = self._filter
   }
   self._data_binding = data_binding
   self._data_binding:update(self._data)

   self._destination:set_region(_radiant.sim.alloc_region())
                    :set_auto_update_adjacent(true)

   radiant.events.listen(radiant.events, 'stonehearth:gameloop', self, self.on_gameloop)
   all_stockpiles[self._entity:get_id()] = self
end

function StockpileComponent:set_filter(values)
   for name, _ in pairs(self._filter) do
      self._filter[name] = false
   end

   for i, name in ipairs(values) do
      self._filter[name] = true
   end
   
   self._data_binding:update(self._data)

   -- for items that no longer match the filter, 
   -- remove then re-add the item from its parent. Other stockpiles
   -- will be notified when it is added to the world and will
   -- reconsider it for placement.
   for _, item in pairs(self._data.items) do
      if not self:can_stock_entity(item) then
         local parent = item:add_component('mob'):get_parent()
         parent:get_component('entity_container'):remove_child(item)
         parent:get_component('entity_container'):add_child(item)
      end
   end

   self:_create_worker_tasks()
   return self
end

-- xxx: the 'fire one when i'm constructed' pattern again...
function StockpileComponent:on_gameloop()
   radiant.events.unlisten(radiant.events, 'stonehearth:gameloop', self, self.on_gameloop)

   self:_create_worker_tasks()

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
   return self._data.items;
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

function StockpileComponent:_get_world_bounds()
   local size = self:get_size()
   local origin = radiant.entities.get_world_grid_location(self._entity)
   local bounds = Cube3(origin, Point3(origin.x + size[1], origin.y + 1, origin.z + size[2]))
   return bounds
end

function StockpileComponent:is_full()
   return false
end

function StockpileComponent:contains(item_entity)
   local location = radiant.entities.get_world_grid_location(item_entity)
   local world_bounds = self:_get_world_bounds()
   return world_bounds:contains(location) and self:can_stock_entity(item_entity)
end

function StockpileComponent:get_size()
   return { self._data.size[1], self._data.size[2] }
end

function StockpileComponent:set_size(size)
   self._data.size = { size[1], size[2] }
   self:_rebuild_item_data()
end

--[[
function StockpileComponent:reserve_adjacent(location)
   local origin = Point3(radiant.entities.get_world_grid_location(self._entity))
   local pt = Point3(location) - origin
   local points = {
      Point3(pt.x + 1, pt.y, pt.z),
      Point3(pt.x - 1, pt.y, pt.z),
      Point3(pt.x, pt.y, pt.z + 1),
      Point3(pt.x, pt.y, pt.z - 1),
   }
   local region = self._destination:get_region()
   for _, p in ipairs(points) do
      if region:contains(p) then
         self:_remove_world_point_from_region(p)
         return p
      end
   end
end
]]--

function StockpileComponent:reserve(location)
   local origin = Point3(radiant.entities.get_world_grid_location(self._entity))
   local pt = Point3(location) - origin
   local region = self._destination:get_region()
   if region:get():contains(pt) then
      region:modify(function(cursor)
         cursor:subtract_point(pt)
      end)
      return true
   end
end

function StockpileComponent:_remove_world_point_from_region(location)
   local origin = radiant.entities.get_world_grid_location(self._entity)
   local offset = location - origin
   local region = self._destination:get_region()
   region:modify(function(cursor)
      cursor:subtract_point(offset)
   end)
   if self._pickup_task then
      if region:get():empty() then
         self._pickup_task:stop()
      else
         self._pickup_task:start()
      end
   end
end

function StockpileComponent:_add_item(entity)
   local location = radiant.entities.get_world_grid_location(entity)
   local world_bounds = self:_get_world_bounds()

      
   if self:can_stock_entity(entity) and self:contains(entity) then
      local item = entity:get_component('item')
      if item then
         -- hold onto the item...
         self._data.items[entity:get_id()] = entity

         -- add the item to the inventory 
         radiant.events.trigger(self._entity, "stonehearth:item_added", { 
            storage = self._entity,
            item = entity 
         })

         -- update our destination component to *remove* the space
         -- where the item is, since we can't drop anything there
         self:_remove_world_point_from_region(location)
         self._data_binding:mark_changed()
      end
   end
end

function StockpileComponent:_remove_item(id)
   local entity = self._data.items[id]
   if entity then
      self._data.items[id] = nil
      self._data_binding:mark_changed()

      -- add this point to our destination region
      local region = self._destination:get_region()
      local origin = radiant.entities.get_world_grid_location(entity)
      --TODO: find source bug; sometimes by here,
      --entity has been sufficiently disposed of that we find out where it is to reclaim its space.
      if origin then 
         local offset = origin - radiant.entities.get_world_grid_location(self._entity)
         region:modify(function(cursor)
            cursor:add_point(Point3(offset))
         end)
      end
   end
end

function StockpileComponent:_rebuild_item_data()
   local region = self._destination:get_region()
   region:modify(function(cursor)
      cursor:clear()
      cursor:add_cube(self:_get_bounds())
   end)

   self._data.items = {}
   self._data_binding:mark_changed()

   local ec = radiant.entities.get_root_entity()
                  :get_component('entity_container')

   for id, child in ec:each_child() do
      self:_add_item(child)
   end
end

function StockpileComponent:_assign_to_faction()
   local faction = self._entity:add_component('unit_info'):get_faction()

   if not self._faction or self._faction ~= faction then
      if self._faction then
         -- unregister from the current inventory service
         local inventory = inventory_service:get_inventory(self._faction)
         inventory:remove_storage(self._entity)
      end

      -- register with the inventory service for this faction
      if faction then
         local inventory = inventory_service:get_inventory(faction)
         inventory:add_storage(self._entity)
      end
   end

   self._faction = faction
end


--- Returns whether or not the stockpile should stock this entity
-- @param entity The entity you're interested in
-- @return true if the entity can be stocked here, false otherwise.

function StockpileComponent:can_stock_entity(entity)
   if not entity or not entity:get_component('item') then
      return false
   end
   
   local material = entity:get_component('stonehearth:material')
   if not material then
      return false
   end

   local in_filter = false
   for name, value in pairs(self._filter) do
      if value and material:is(name) then
         return true
      end
   end
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

   local faction = radiant.entities.get_faction(self._entity)
   local worker_scheduler = radiant.mods.load('stonehearth').worker_scheduler:get_worker_scheduler(faction)

   if self._pickup_task then
      worker_scheduler:remove_worker_task(self._pickup_task)
      self._pickup_task:destroy()
   end
   if self._restock_task then
      worker_scheduler:remove_worker_task(self._restock_task)
      self._restock_task:destroy()
   end

   -- This is the pickup task.  When it finishes, we want to run the
   -- stonehearth:pickup_item_on_path action, passing in the path found
   -- by the worker task.
   self._pickup_task = worker_scheduler:add_worker_task('pickup_to_restock')
                          :set_action('stonehearth:pickup_item_on_path')
                          :set_priority(priorities.RESTOCK_STOCKPILE)

   -- Only consider workers that aren't currently carrying anything.
   self._pickup_task:set_worker_filter_fn(
      function(worker)
         return radiant.entities.get_carrying(worker) == nil
      end
   )

   -- Only consider items which belong in this stockpile and are not
   -- in any other stockpiles!
   self._pickup_task:set_work_object_filter_fn(
      function(entity)
         if not self:can_stock_entity(entity) then
            return false
         end

         local stockpile = get_stockpile_containing_entity(entity)

         if stockpile and not stockpile:is_outbox() then 
            return false
         end
         
         return true
      end
   )

   -- Next is the restock task, which will actually do the dumping of items
   -- into the stockpile.
   self._restock_task = worker_scheduler:add_worker_task('restock_stockpile')
                                        :set_priority(priorities.RESTOCK_STOCKPILE)

   -- We should only consider workers that are carrying and item that belongs
   -- in the stockpile AND workers who aren't already standing in a stockpile
   -- (since they've probably just picked up something from that stockpile)
   -- TODO: Ask Tony if this counts as a hack
   -- Yes, this is a horrible hack.  OMG!!  The way to fix it is to make restocking
   -- stuff the lowest level task and make sure people use a "one task for the whole
   -- pipeline" pattern like place item does.  Then the restock pathfinder will never
   -- get a crack at people who pull stuff out of the stockpile to do other things.
   --  - tony
   
   self._restock_task:set_worker_filter_fn(
      function (worker)
         local entity = radiant.entities.get_carrying(worker)
         if not self:can_stock_entity(entity) then
            return false
         end
         --TODO: what if the worker is standing just outside the stockpile?
         return not self:contains(worker)
      end
   )

   -- We want to drop stuff off inside ourselves, so add self as a destination
   self._restock_task:add_work_object(self._entity)

   -- We need to pass ourselves to the restock action, so we can't use the
   -- 'set_action' shortcut.  Register via 'set_action_fn' which lets us construct
   -- whatever kind of activity we want
   self._restock_task:set_action_fn(
      function (path)
         return 'stonehearth:restock', path, self
      end
   )

   -- fire 'em up!
   self._pickup_task:start()
   self._restock_task:start()

end

return StockpileComponent
