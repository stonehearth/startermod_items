local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3

local Inventory = class()

--Keeps track of the stuff in a player's stockpiles

function Inventory:__init()
end

function Inventory:initialize(session)   
   self._sv.next_stockpile_no = 1
   self._sv.player_id = session.player_id
   self._sv.kingdom = session.kingdom
   self._sv.faction = session.faction
   self._sv.storage = {}
   self._sv.items = {}
   self._sv.trackers = {}
   self.__saved_variables:mark_changed()

   self:add_item_tracker('stonehearth:basic_inventory_tracker')
end

function Inventory:restore()
end

function Inventory:create_stockpile(location, size)
   local entity = radiant.entities.create_entity('stonehearth:stockpile')   
   radiant.terrain.place_entity(entity, location)
   entity:get_component('stonehearth:stockpile'):set_size(size.x, size.y)

   self:_add_collision_region(entity, size)

   --xxx localize
   entity:get_component('unit_info'):set_display_name('Stockpile No.' .. self._sv.next_stockpile_no)
   entity:get_component('unit_info'):set_player_id(self._sv.player_id)
   entity:get_component('unit_info'):set_faction(self._sv.faction)

   self._sv.next_stockpile_no = self._sv.next_stockpile_no + 1
   self.__saved_variables:mark_changed()
   return entity
end

function Inventory:get_all_storage()
   return self._sv.storage
end

function Inventory:add_storage(storage_entity)
   assert(not self._sv.storage[storage_entity:get_id()])
   self._sv.storage[storage_entity:get_id()] = storage_entity
   self.__saved_variables:mark_changed()
   radiant.events.trigger(self, 'stonehearth:storage_added', { storage = storage_entity })
end

function Inventory:remove_storage(storage_entity)
   local id = storage_entity:get_id()
   if self._sv.storage[id] then
      self._sv.storage[id] = nil
      self.__saved_variables:mark_changed()
   end
   radiant.events.trigger(self, 'stonehearth:storage_removed', { storage = storage_entity })
   --xxx remove the items?
end

function Inventory:_add_collision_region(entity, size)
   local collision_component = entity:add_component('region_collision_shape')
   local collision_region_boxed = _radiant.sim.alloc_region()

   collision_region_boxed:modify(
      function (region3)
         region3:add_unique_cube(
            Cube3(
               -- recall that region_collision_shape is in local coordiantes
               Point3(0, 0, 0),
               Point3(size.x, 1, size.y)
            )
         )
      end
   )

   collision_component:set_region(collision_region_boxed)
   collision_component:set_region_collision_type(_radiant.om.RegionCollisionShape.NONE)
end

--- Call whenever a stockpile wants to tell the inventory that we're adding an item
function Inventory:add_item(storage, item)
   local storage_id = storage:get_id()
   assert(self._sv.storage[storage_id], 'tried to add an item to an untracked storage entity ' .. tostring(storage))

   self._sv.items[item:get_id()] = item
   self.__saved_variables:mark_changed()

   --Tell all the trackers for this player about this item
   for name, tracker in pairs(self._sv.trackers) do
      tracker:add_item(item)
   end

   radiant.events.trigger(self, 'stonehearth:item_added', { item = item })
end

--- Call whenever a stockpile wants to tell the inventory that we're removing an item
function Inventory:remove_item(storage, item_id)
   local storage_id = storage:get_id()
   assert(self._sv.storage[storage_id], 'tried to remove an item to an untracked storage entity ' .. tostring(storage))

   if self._sv.items[item_id] then
      self._sv.items[item_id] = nil
      self.__saved_variables:mark_changed()

   --Tell all the trackers for this player about this item
   for name, tracker in pairs(self._sv.trackers) do
      tracker:remove_item(item_id)
   end
      radiant.events.trigger(self, 'stonehearth:item_removed', { item_id = item_id })
   end
end

--- Call this function to track a subset of things in this inventory
--  See the documentation for InventoryTracker for the function specifics
function Inventory:add_item_tracker(controller_name)
   local tracker = self._sv.trackers[controller_name]
   if not tracker then
      local controller = radiant.create_controller(controller_name)
      assert(controller)
      
      tracker = radiant.create_controller('stonehearth:inventory_tracker', controller)
      for id, item in pairs(self._sv.items) do
         tracker:add_item(item)
      end
      self._sv.trackers[controller_name] = tracker
      self.__saved_variables:mark_changed()
   end
   return tracker
end

function Inventory:get_item_tracker(controller_name)
   return self._sv.trackers[controller_name]
end

--- Given the uri of an item and the player_id, get a structure containing items of that type
--  Uses the basic_inventory_tracker
--
--  @param item_type : uri of the item (like stonehearth:oak_log)
--  @param player_id : id of the player
--  @returns an object with a count and a map of identity (items).
--           Iterate through the map to get the data.
--           If entity is nil, that item in the map is now invalid. If the data is nil, there was
--           nothing of that type
--
function Inventory:get_items_of_type(uri)
   local tracker = self:get_item_tracker('stonehearth:basic_inventory_tracker')
   local tracking_data = tracker:get_tracking_data()
   return tracking_data[uri]
end

-- call the `cb` function whenever the inventory receives a new item matching `uri`
--
-- @param uri - looks for entities with the given uri
-- @param cb - the callback function to call when an item is added
--
function Inventory:notify_when_item_added(uri, cb)
   local tracker = self:get_item_tracker('stonehearth:basic_inventory_tracker')
   return radiant.events.listen(tracker, 'stonehearth:inventory_tracker:item_added', function (e)
         if e.key == uri then
            cb()
         end
      end)
end

-- find a placeable item whose iconic form has the given `uri` which is not currently
-- being placed.  if multiple items are not being placed, return the one closest to
-- `location`.  also returns the total number of items which match `uri` and are not
-- currently placed as the 2nd result.
--
-- @param uri - the uri of the iconic entity to look for
-- @param location - of all the candiates, will return the one closest to location
--
function Inventory:find_closest_unused_placable_item(uri, location)
   -- look for entities which are not currently being placed
   local candidates = self:get_items_of_type(uri)
   if not candidates then
      return
   end
   
   -- returns the best item to place.  the best item is the one that isn't currently
   -- being placed and is closest to the placement location
   local acceptable_item_count = 0
   local best_item, best_distance

   for _, item in pairs(candidates.items) do
      -- make sure the item isn't being placed
      local entity_forms = item:get_component('stonehearth:iconic_form')
                                 :get_root_entity()
                                 :get_component('stonehearth:entity_forms')
      if not entity_forms:is_being_placed() then
         acceptable_item_count = acceptable_item_count + 1
         local position = radiant.entities.get_world_grid_location(item)
         local distance = position:distance_to(location)

         -- make sure the item is better than the previous one.
         if not best_item or distance < best_distance then
            best_item, best_distance = item, distance
         end
      end
   end
   return best_item, acceptable_item_count
end

return Inventory
