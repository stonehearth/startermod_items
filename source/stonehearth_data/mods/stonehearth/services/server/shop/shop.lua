local Entity = _radiant.om.Entity

local entity_forms = require 'stonehearth.lib.entity_forms.entity_forms_lib'
local Shop = class()

function Shop:__init()
end

function Shop:initialize(player_id)
   local inventory = stonehearth.inventory:get_inventory(player_id)

   self._sv.player_id = player_id
   self._sv.options = {}
   self._sv.level_range = { min = -1, max = -1}
   self._sv.sellable_items = inventory:get_item_tracker('stonehearth:sellable_item_tracker')
   self._sv._inventory = inventory
   self.__saved_variables:mark_changed()
end

function Shop:activate()
   local inventory = self._sv._inventory
   self._gold_trace = inventory:trace_gold('updating shop')
                                    :on_changed(function(gold)
                                          self._sv.gold = gold
                                          self.__saved_variables:mark_changed()
                                       end)
                                    :push_object_state()

end

function Shop:destroy()
   if self._gold_trace then
      self._gold_trace:destroy()
      self._gold_trace = nil
   end
end

function Shop:get_name()
   return self._sv.name
end

function Shop:set_name(name)
   self._sv.name = name
   self.__saved_variables:mark_changed()
   return self
end

--- Options
--  {
--     "filters" : [
--        {
--           item_category : ["furniture", "decoration"]
--           item_material : ["wood"]
--        }
--     ],
--     "items" : [
--       "stonehearth:refined:thread"
--     ]
--  }
function Shop:set_options(options)
   self._sv.options = options or {}
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

---A function which implements the default item filter for a shop.  
function Shop:item_filter_fn(entity)
   local inventory_spec = self._sv.options

   -- check if the entity in the list of items
   if inventory_spec.items then
      local entity_path = radiant.resources.convert_to_canonical_path(entity:get_uri())
      for _, uri in ipairs(inventory_spec.items) do
         local item_path = radiant.resources.convert_to_canonical_path(uri)
         if item_path == entity_path then
            return true
         end
      end
   end

   -- check if the entity passes any filter
   if inventory_spec.items_matching then
      for key, filter in pairs(inventory_spec.items_matching) do
         if self:_inventory_filter_fn(entity, filter) then
            return true
         end
      end
   end

   return false
end

function Shop:_inventory_filter_fn(entity, filter)
   if not self:_item_in_category_filter(entity, filter) then
      return false
   end

   if not self:_item_in_material_filter(entity, filter) then
      return false
   end

   return true
end

function Shop:_item_in_category_filter(entity, filter)
   
   if not filter.category then 
      return true
   end 

   local entity_category = radiant.entities.get_category(entity)
   for _, category in pairs(filter.category) do
      if entity_category == category then
         return true
      end
   end

   return false   
end

function Shop:_item_in_material_filter(entity, filter)
   if not filter.material then
      return true
   end

   for _, material in pairs(filter.material) do
      if material then
         return radiant.entities.is_material(entity, material)
      end
   end

   return true   
end

function Shop:stock_shop() 
   -- get a table of all the items which can appear in a shop
   local all_sellable_items = stonehearth.shop:get_sellable_items()

   self.sellable_items = {}
   self._sv.shop_inventory = {}

   -- Copy the items which pass the filter function into a new table, with buckets for rarity.
   for uri, entity in pairs(all_sellable_items) do
      if self:item_filter_fn(entity) then
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

         local unit_info = shop_item:get_component('unit_info')
         local item = shop_item:get_component('item')
         local category = radiant.entities.get_category(shop_item)

         self._sv.shop_inventory[uri] = {
            uri = uri,
            display_name = unit_info:get_display_name(),
            description = unit_info:get_description(),
            icon = unit_info:get_icon(),
            item = shop_item,
            cost = item_cost,
            category = category,
            num = 99,
         }
      end
      -- Repeat until the total combined cost of all the items exceeds the Shops max inventory net worth
   end
   self.__saved_variables:mark_changed()
end

function Shop:buy_item(uri, quantity)
   -- is the uri actually in the shop inventory?
   if not uri or not self._sv.shop_inventory[uri] then
      return
   end

   local buy_quantity = quantity or 1
   local all_sellable_items = stonehearth.shop:get_sellable_items()
   
   -- do we have enough gold?
   local gold = self._sv._inventory:get_gold_count()
   local item_cost = self._sv.shop_inventory[uri].cost

   -- can't buy more than what's in the shop
   if buy_quantity > self._sv.shop_inventory[uri].num then
      buy_quantity = self._sv.shop_inventory[uri].num
   end

   -- if we can't afford as many as we want, buy fewer
   buy_quantity = math.min(buy_quantity, math.floor(gold / item_cost))

   -- if we can't afford it, bail
   if buy_quantity == 0 then
      return
   end

   local total_cost = item_cost * buy_quantity

   -- remove the items from my inventory
   self._sv.shop_inventory[uri].num = self._sv.shop_inventory[uri].num - buy_quantity
   self.__saved_variables:mark_changed()

   -- deduct gold from the player
   self._sv._inventory:subtract_gold(total_cost)

   -- spawn the item on the ground
   -- TODO: spawn items into escrow storage container; dump escrow storage container when dialog closes
   self:_spawn_items(uri, buy_quantity)
   return true
end

function Shop:buy_item_command(session, response, uri, quantity)
   local success = self:buy_item(uri, quantity)
   if success then
      response:resolve({})
      return
   end
   response:reject({})
end

function Shop:sell_item(uri, quantity)
   local sell_quantity = quantity or 1

   --the items we can sell should come from the sellable item tracker, not the whole inventory
   local tracking_data = self._sv.sellable_items:get_tracking_data()
   local sellable_items =  tracking_data[uri]

   local item_cost
   local total_gold = 0

   -- "sell" each entity by destroying it, until we've sold the requested amount or run out of entities
   for uri, entity in pairs(sellable_items.items) do
      if sell_quantity == 0 then
         break
      end
      if item_cost == nil then
         -- _get_item_cost needs an entity, so we can't call it until we have one
         -- (e.g. if uri is an iconic entity, we can't get the cost intil we have the actual
         -- iconic_entity to get it's iconic_form component to get back to the real entity!)
         local entity_key = entity:get_uri()
         assert(tracking_data, 'sure as heck better having tracking data for sale item %s', entity_key)
         item_cost = tracking_data[entity_key].resale
      end
      radiant.entities.destroy_entity(entity)
      total_gold = total_gold + item_cost
      sell_quantity = sell_quantity - 1
   end

   -- give gold to the player
   self._sv._inventory:add_gold(total_gold)
   return true
end

function Shop:sell_item_command(session, response, uri, quantity)
   local success = self:sell_item(uri, quantity)
   if success then
      response:resolve({})
   else 
      response:reject({})
   end
end

function Shop:_spawn_items(uri, quantity)
   --Add the new items to the space near the banner
   local town = stonehearth.town:get_town(self._sv.player_id)
   local banner = town:get_banner()
   local drop_origin = banner and radiant.entities.get_world_grid_location(banner)
   if not drop_origin then
      return false
   end

   local items = {}
   items[uri] = quantity
   items = radiant.entities.spawn_items(items, drop_origin, 1, 3, { owner = self._sv.player_id })

   ---[[
   -- TODO: eventually, pause the game while we're in the shop. 
   -- Handle all shop transactions during pause, then add gold/remove items from the game
   -- 
   local inventory = self._sv._inventory
   for _, item in pairs(items) do
      local root, iconic, _ = entity_forms.get_forms(item)
      if iconic then
         inventory:add_item(iconic)
      elseif root then
         inventory:add_item(root)
      end
   end
   --]]
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
   assert(radiant.util.is_a(entity, Entity))
   local entity_uri, _, _ = entity_forms.get_uris(entity)
   local net_worth = radiant.entities.get_entity_data(entity_uri, 'stonehearth:net_worth')
   if not net_worth then
      return 0
   end
   return net_worth.value_in_gold
end

function Shop:_get_category(entity)
   local item = entity:get_component('item')
   local category = 'nil'

   if item then
      category = item:get_category()
   end

end
return Shop
