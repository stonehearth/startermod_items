local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Point3 = _radiant.csg.Point3

local Inventory = class()

--Keeps track of the stuff in a player's stockpiles

function Inventory:__init()
end

function Inventory:initialize(session)   
   self._sv.next_stockpile_no = 1
   self._sv.player_id = session.player_id
   self._sv.stockpiles = {}
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
   entity:get_component('stonehearth:stockpile')
            :set_size(size.x, size.y)

   entity:add_component('region_collision_shape')
            :set_region_collision_type(_radiant.om.RegionCollisionShape.NONE)
            :set_region(_radiant.sim.alloc_region3())
            :get_region():modify(function (cursor)
                  cursor:add_unique_cube(Cube3(Point3.zero, Point3(size.x, 1, size.y)))
                  end)

   --xxx localize
   entity:get_component('unit_info')
            :set_display_name('Stockpile No.' .. self._sv.next_stockpile_no)
            :set_player_id(self._sv.player_id)

   self._sv.next_stockpile_no = self._sv.next_stockpile_no + 1
   self.__saved_variables:mark_changed()
   return entity
end

--Returns a map of the stockpiles, sorted by ID
function Inventory:get_all_stockpiles()
   return self._sv.stockpiles
end

function Inventory:add_stockpile(stockpile)
   assert(not self._sv.stockpiles[stockpile:get_id()])
   self._sv.stockpiles[stockpile:get_id()] = stockpile
   self.__saved_variables:mark_changed()
   radiant.events.trigger(self, 'stonehearth:inventory:stockpile_added', { stockpile = stockpile })
end

function Inventory:remove_stockpile(stockpile)
   local id = stockpile:get_id()
   if self._sv.stockpiles[id] then
      self._sv.stockpiles[id] = nil
      self.__saved_variables:mark_changed()
   end
   radiant.events.trigger(self, 'stonehearth:inventory:stockpile_removed', { stockpile = stockpile })
   --xxx remove the items?
end

--- Call whenever a stockpile wants to tell the inventory that we're adding an item
function Inventory:add_item(item)
   self._sv.items[item:get_id()] = item
   --Tell all the trackers for this player about this item
   for name, tracker in pairs(self._sv.trackers) do
      tracker:add_item(item)
   end
   radiant.events.trigger(self, 'stonehearth:inventory:item_added', { item = item })
   self.__saved_variables:mark_changed()

end

--- Call whenever a stockpile wants to tell the inventory that we're removing an item
function Inventory:remove_item(item_id)
   if self._sv.items[item_id] then
      self._sv.items[item_id] = nil
      
      --Tell all the trackers for this player about this item
      for name, tracker in pairs(self._sv.trackers) do
         tracker:remove_item(item_id)
      end
      radiant.events.trigger(self, 'stonehearth:inventory:item_removed', { item_id = item_id })
      self.__saved_variables:mark_changed()
   end
end

--- Call this function to track a subset of things in this inventory
--  If no such tracker exists, make a new one. If one exists, use it. 
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

function Inventory:get_gold_count_command(session, response, uri)
   local gold = self:get_gold_count()
   response:resolve({ gold = gold })
end

function Inventory:get_gold_count()
   local gold_items = self:get_items_of_type('stonehearth:loot:gold')

   local total_gold = 0
   if gold_items ~= nil then
      for id, item in pairs(gold_items.items) do
         -- get stacks for the item
         local item_component = item:add_component('item')
         local stacks = item_component:get_stacks()

         total_gold = total_gold + stacks
      end
   end

   return total_gold
end

function Inventory:add_gold(amount)
   -- for now, always add the gold in a new entity, instead of adding stacks to
   -- and existing entity

   --Add the new items to the space near the banner
   local town = stonehearth.town:get_town(self._sv.player_id)
   local banner = town:get_banner()
   local drop_origin = banner and radiant.entities.get_world_grid_location(banner)
   if not drop_origin then
      return false
   end

   local gold = radiant.entities.create_entity('stonehearth:loot:gold')
   local item = gold:add_component('item')
   item:set_stacks(amount)

   local location = radiant.terrain.find_placement_point(drop_origin, 1, 3)
   radiant.terrain.place_entity(item, location)
end

function Inventory:subtract_gold(amount)
   local gold_items = self:get_items_of_type('stonehearth:loot:gold')
   local stacks_to_remove = amount

   if gold_items ~= nil then
      for id, item in pairs(gold_items.items) do
         -- get stacks for the item
         local item_component = item:add_component('item')
         local item_stacks = item_component:get_stacks()

         -- nuke some stacks
         if item_stacks > stacks_to_remove then
            -- this item has more stacks than we need to remove, reduce the stacks and we're done
            item_component:set_stacks(item_stacks - stacks_to_remove)
            break
         else 
            -- consume the whole item and run through the loop again
            radiant.entities.destroy_entity(item)
            stacks_to_remove = stacks_to_remove - item_stacks
         end

         assert(stacks_to_remove >= 0)

         if stacks_to_remove == 0 then
            break
         end
      end
   end
end

return Inventory
