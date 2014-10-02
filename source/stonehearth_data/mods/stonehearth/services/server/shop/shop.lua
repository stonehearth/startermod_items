local Shop = class()

function Shop:__init()
end

function Shop:initialize(session)   
   self._sv = self.__saved_variables:get_data()
   self._session = session
   if not self._sv.initialized then
      self._sv.level_range = { min = -1, max = -1}
      self.__saved_variables:mark_changed()
   end
end

---Sets the function which determines whether or not an item should be available 
-- in the shop.  The function gets passed an instance of that item and the shop itself
function Shop:set_item_filter_fn(fn)
   self._sv.item_filter_fn = fn
   self.__saved_variables:mark_changed()
   return self
end

---Gets the level range of the shopkeeper for this shop.  The getter 
-- returns 2 values (the min and the max).
function Shop:get_shopkeeper_level_range()
   return nil
end


function Shop:set_shopkeeper_level_range(range)
   return self
end

---Returns a table of bools (e.g. { junk = true, common = true }
function Shop:get_inventory_rarity()
   return nil
end

---Sets the rarity of the items which should be in the shop.  
function Shop:set_inventory_rarity(rarities)
   return self
end

---Sets or gets the maximum value of all the items in the shop.  Used by Shop:stock_shop to stock up!
function Shop:get_inventory_max_net_worth()
   return nil
end

---Sets or gets the maximum value of all the items in the shop.  Used by Shop:stock_shop to stock up!
function Shop:set_inventory_max_net_worth()
   return self
end

function Shop:stock_shop() 
   -- get a table of all the items which can appear in a shop
   local all_sellable_items = stonehearth.shop:get_sellable_items()

   self.sellable_items = {}
   self._sv.inventory = {}

   -- Copy the items which pass the filter function into a new table, with buckets for rarity.
   for uri, entity in pairs(all_sellable_items) do
      if self._sv.item_filter_fn(entity) then
         self:_add_entity_to_sellable_items(uri, entity)
      end
   end

   -- Pick a random bucket based on some metric (e.g. 1% chance or rare bucket, 10% chance of uncommon bucket, etc.)
   -- xxx, for now 1 from each bucket
   for rarity, entities in pairs(self.sellable_items) do
      -- Pick a random item in the bucket
      -- xxx, for now do them all.
      for uri, shop_item in pairs(entities) do
         -- Add it to the shop inventory
         local item_cost = radiant.entities.get_entity_data(shop_item, 'stonehearth:net_worth').value_in_gold
         self._sv.inventory[uri] = {
            uri = uri,
            item = shop_item,
            cost = item_cost,
            num = 99,
         }
      end

      -- Repeat until the total combined cost of all the items exceeds the Shops max inventory net worth
   end

   local inventory = stonehearth.inventory:get_inventory(self._session.player_id)
   self._sv.player_inventory = inventory:add_item_tracker('stonehearth:shop:sellable_item_tracker')

   self.__saved_variables:mark_changed()
end

function Shop:buy_item(uri, quantity)
   local buy_quantity = quantity or 1

   -- do we have enough gold?
   local inventory = stonehearth.inventory:get_inventory(self._session.player_id)
   local gold = inventory:get_gold_count()
   local item_cost = self._sv.inventory[uri].cost

   -- can't buy more than what's in the shop
   if buy_quantity > self._sv.inventory[uri].num then
      buy_quantity = self._sv.inventory[uri].num
   end

   -- if we can't afford as many as we want, buy fewer
   buy_quantity = math.min(buy_quantity, math.floor(gold / item_cost))

   -- if we can't afford it, bail
   if buy_quantity == 0 then
      return
   end

   local total_cost = item_cost * buy_quantity

   -- remove the items from my inventory
   self._sv.inventory[uri].num = self._sv.inventory[uri].num - buy_quantity
   self.__saved_variables:mark_changed()

   -- deduct gold from the player
   inventory:subtract_gold(total_cost)

   -- spawn the item on the ground
   self:_spawn_items(uri, buy_quantity)
   return true
end

function Shop:buy_item_command(session, response, uri, quantity)
   local success = self:buy_item(uri, quantity)
   if success then
      response:resolve({})
   else 
      response:reject({})
   end
end

function Shop:_spawn_items(uri, quantity)
   --Add the new items to the space near the banner
   local town = stonehearth.town:get_town(self._session.player_id)
   local banner = town:get_banner()
   local drop_origin = banner and radiant.entities.get_world_grid_location(banner)
   if not drop_origin then
      return false
   end

   local items = {}
   items[uri] = quantity
   radiant.entities.spawn_items(items, drop_origin, 1, 3, self._session.player_id)

   return true
end

function Shop:_add_entity_to_sellable_items(uri, entity)
   local rarity = 'junk'
   local net_worth = radiant.entities.get_entity_data(entity, 'stonehearth:net_worth')
   if net_worth and net_worth.rarity then
      rarity = net_worth.rarity
   end

   if self.sellable_items[rarity] == nil then
      self.sellable_items[rarity] = {}
   end

   self.sellable_items[rarity][uri] = entity
end

function Shop:_get_item_cost(entity)
   return radiant.entities.get_entity_data(entity, 'stonehearth:net_worth').value_in_gold
end
return Shop
