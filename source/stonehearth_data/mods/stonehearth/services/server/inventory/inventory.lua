local entity_forms_lib = require 'lib.entity_forms.entity_forms_lib'
local FilteredTrace = require 'radiant.modules.filtered_trace'

local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Point3 = _radiant.csg.Point3

local Inventory = class()

--Keeps track of the stuff in a player's stockpiles

function Inventory:__init()
end

local SHARED_STORAGE  = 'shared'
local PRIVATE_STORAGE = 'private'

function Inventory:initialize(player_id)   
   self._sv.next_stockpile_no = 1
   self._sv.player_id = player_id
   self._sv.stockpiles = {}
   self._sv.items = {}
   self._sv.trackers = {}
   self._sv.container_for = {}
   self._sv.storage = {
      [SHARED_STORAGE] = {},
      [PRIVATE_STORAGE] = {},
   }
   self._sv.public_storage_is_full = true
   self.__saved_variables:mark_changed()

   self:add_item_tracker('stonehearth:basic_inventory_tracker')
   self:add_item_tracker('stonehearth:placeable_item_inventory_tracker')
   self:add_item_tracker('stonehearth:sellable_item_tracker')
end

function Inventory:activate()
   self:_install_listeners()
end

function Inventory:destroy()
   if self._destroy_listener then
      self._destroy_listener:destroy()
      self._destroy_listener = nil
   end
   if self._stacks_changed_listener then
      self._stacks_changed_listener:destroy()
      self._stacks_changed_listener = nil
   end
   if self._add_item_to_score_listener then
      self._add_item_to_score_listener:destroy()
      self._add_item_to_score_listener = nil
   end
   if self._remove_item_from_score_listener then
      self._remove_item_from_score_listener:destroy()
      self._remove_item_from_score_listener = nil
   end
end

function Inventory:_install_listeners()
   self._destroy_listener = radiant.events.listen(radiant, 'radiant:entity:pre_destroy', self, self._on_destroy)

   self._stacks_changed_listener = radiant.events.listen(radiant, 'radiant:item:stacks_changed', function(e)
         self:_update_score_for_item(e.entity)
      end)
end

function Inventory:_update_score_for_item(item)
   if item and item:is_valid() then
      -- if this is an icon, find the root entity
      local ic = item:get_component('stonehearth:iconic_form')
      if ic then
         item = ic:get_root_entity()
         if not item or not item:is_valid() then
            return
         end
      end

      -- compute the score
      local score = 0
      local id = item:get_id()
      score = self:_get_score_for_item(item) / 10 -- whyyyyyyyyy? -- tony
      
      -- add to 'edibles' and 'net_worth' categories
      local mc = item:get_component('stonehearth:material')
      if mc and (mc:is('food') or mc:is('food_container')) then
         stonehearth.score:change_score(item, 'edibles', 'inventory', score)
      end
      stonehearth.score:change_score(item, 'net_worth', 'inventory', score)
   end
end


--- Given an entity, get the score for it, aka the entity's net worth 
--  If we don't have a score for that entity, use the default score, which is 1
function Inventory:_get_score_for_item(item)
   -- resources that are not food have no score.
   local mc = item:get_component('stonehearth:material')
   if mc then
      if mc:is('resource') and not (mc:is('food') or mc:is('food_container')) then
         return 0
      end
   end

   local networth = radiant.entities.get_entity_data(item, 'stonehearth:net_worth')
   if networth and networth.value_in_gold then
      -- The value of wealth is item_score * stacks
      local ic = item:get_component('item')
      if ic and ic:get_category() == 'wealth' then
         local stacks = ic:get_stacks()
         return networth.value_in_gold * stacks
      end
      --if not wealth, then just return value in gold
      return networth.value_in_gold
   end

   -- otherwise, return 1
   return 1
end


function Inventory:_on_destroy(e)
   local id = e.entity_id
   if self._sv.items[id] then
      self:remove_item(id)
   end

   for t, storage in pairs(self._sv.storage) do
      if storage[id] then
         storage[id] = nil
         self.__saved_variables:mark_changed()
         if t == SHARED_STORAGE then
            self:_check_public_storage_space()
         end
      end
   end
end

function Inventory:add_storage(entity)
   local id = entity:get_id()
   local is_public = entity:get_component('stonehearth:storage')
                                 :is_public()

   local typ = is_public and SHARED_STORAGE or PRIVATE_STORAGE
   local storage = self._sv.storage[typ]
   if not storage[id] then
      storage[id] = entity
      if is_public then
         self:_check_public_storage_space()
      end
      self.__saved_variables:mark_changed()
   end
end

function Inventory:public_storage_is_full()
   return self._sv.public_storage_is_full
end

function Inventory:_check_public_storage_space()
   local is_full = function()
      for id, storage in pairs(self._sv.storage[SHARED_STORAGE]) do
         local sc = storage:get_component('stonehearth:storage')
         if not sc:is_full() then
            return false
         end
      end
      return true
   end
   local full = is_full()
   self:_set_public_storage_is_full(full)
end

function Inventory:_set_public_storage_is_full(value)
   if self._sv.public_storage_is_full ~= value then
      self._sv.public_storage_is_full = value
      self.__saved_variables:mark_changed()
      radiant.events.trigger_async(self, 'stonehearth:inventory:public_storage_full_changed')
   end
end

function Inventory:create_stockpile(location, size)
   local entity = radiant.entities.create_entity('stonehearth:stockpile', { owner = self._sv.player_id })
   radiant.terrain.place_entity(entity, location)

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

   entity:add_component('stonehearth:storage')
            :set_capacity(nil)

   entity:add_component('stonehearth:stockpile')
            :set_size(size.x, size.y)

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
function Inventory:add_item(item, storage)
   local id = item:get_id()
   local items = self._sv.items

   item:add_component('unit_info')
            :set_player_id(self._sv.player_id)

   if items[id] then
      self:update_item_container(id, storage)
      return
   end
   
   assert(not self._sv.container_for[id])

   items[id] = item

   -- update the container map
   if storage then
      self._sv.container_for[id] = storage
   end

   --Tell all the trackers for this player about this item
   for name, tracker in pairs(self._sv.trackers) do
      tracker:add_item(item, storage)
   end

   -- update the score
   self:_update_score_for_item(item)

   radiant.events.trigger(self, 'stonehearth:inventory:item_added', { item = item })
   self.__saved_variables:mark_changed()
   self:_check_public_storage_space()
end

--- Call whenever a stockpile wants to tell the inventory that we're removing an item
--  If the item isn't in the inventory, ignore
function Inventory:remove_item(item_id)
   if self._sv.items[item_id] then
      local item = self._sv.items[item_id]

      self._sv.items[item_id] = nil
      self._sv.container_for[item_id] = nil

      --Tell all the trackers for this player about this item
      for name, tracker in pairs(self._sv.trackers) do
         tracker:remove_item(item_id)
      end

      -- update the score
      if item:is_valid() then
         self:_update_score_for_item(item)
      end

      radiant.events.trigger(self, 'stonehearth:inventory:item_removed', { item_id = item_id })
      self.__saved_variables:mark_changed()

      if self._sv.public_storage_is_full then
         self:_set_public_storage_is_full(false)
      end
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
      
      local container_for = self._sv.container_for
      tracker = radiant.create_controller('stonehearth:inventory_tracker', controller)
      for id, item in pairs(self._sv.items) do
         tracker:add_item(item, container_for[id])
      end
      self._sv.trackers[controller_name] = tracker
      self.__saved_variables:mark_changed()
   end
   return tracker
end

function Inventory:get_item_tracker(controller_name)
   return self._sv.trackers[controller_name]
end


--- Given the uri of an item get a structure containing items of that type
--  Uses the basic_inventory_tracker
--
--  @param item_type : uri of the item (like stonehearth:resources:wood:oak_log)
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
         if position then
            local distance = position:distance_to(location)

            -- make sure the item is better than the previous one.
            if not best_item or distance < best_distance then
               best_item, best_distance = item, distance
            end
         end
      end
   end
   return best_item, acceptable_item_count
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

   local gold = radiant.entities.create_entity('stonehearth:loot:gold', { owner = self._sv.player_id })
   gold:add_component('item')
            :set_stacks(amount)

   local location = radiant.terrain.find_placement_point(drop_origin, 1, 3)
   radiant.terrain.place_entity(gold, location)

   -- add the item to the inventory immediately so anyone displaying the 'gold' amount
   -- will see it update immediately
   self:add_item(gold)
end

function Inventory:subtract_gold(amount)
   local gold_items = self:get_items_of_type('stonehearth:loot:gold')
   local stacks_to_remove = amount

   if gold_items ~= nil then
      local only_stacks_dirty = true
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
            only_stacks_dirty = false
         end

         assert(stacks_to_remove >= 0)

         if stacks_to_remove == 0 then
            break
         end
      end
      -- it is annoying that we have to do this, but the inventory tracker
      -- doesn't trace the stacks of all the items.  sigh.
      if only_stacks_dirty then
         self:get_item_tracker('stonehearth:basic_inventory_tracker')
                  :mark_changed()
      end
   end
end

-- used to be notified whenenver the player gets or loses gold.  the trace
-- object returned can be registered with a function which will pass you the
-- new amount of gold they have.
--
function Inventory:trace_gold(reason)
   local trace = self:get_item_tracker('stonehearth:basic_inventory_tracker')
                           :trace(reason)

   local last_count = nil   
   local update_gold_fn = function()
      local count = self:get_gold_count()
      if count ~= last_count then
         last_count = count
         return count
      end
   end

   return FilteredTrace(trace, update_gold_fn)
end


function Inventory:container_for(item)
   assert(item)
   return self._sv.container_for[item:get_id()]
end

function Inventory:update_item_container(id, storage)
   checks('self', 'number', '?Entity')
   
   if self._sv.container_for[id] == storage then
      return
   end
  
   self._sv.container_for[id] = storage

   local item = self._sv.items[id]
   if not item then
      return
   end
   --Tell all the trackers for this player about this item
   for name, tracker in pairs(self._sv.trackers) do
      tracker:update_item_container(item, storage)
   end
   self.__saved_variables:mark_changed()
end

return Inventory
