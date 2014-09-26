local Shop = class()

function Shop:__init()
end

function Shop:initialize(session)   
   self._sv = self.__saved_variables:get_data()
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
         self._sv.inventory[uri] = {
            uri = uri,
            item = shop_item,
            num = 99
         }
      end

      -- Repeat until the total combined cost of all the items exceeds the Shops max inventory net worth
   end

   self.__saved_variables:mark_changed()
end

function Shop:buy_item(uri)
   self._sv.inventory[uri].num = self._sv.inventory[uri].num - 1
   self.__saved_variables:mark_changed()
end

function Shop:buy_item_command(session, response, uri)
   self:buy_item(uri)
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

return Shop
