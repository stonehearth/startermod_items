local Entity = _radiant.om.Entity

local ShopService = class()


function ShopService:__init()
end

function ShopService:initialize()   
   self._sv = self.__saved_variables:get_data()
   
   if not self._sv.initialized then
      self._sv.initialized = true
   end
   self:_init_sellable_items()
end

---Creates a new shop with reasonable defaults.  See the shop object for more information.
function ShopService:create_shop(player_id, name, options)
   local shop = radiant.create_controller('stonehearth:shop', player_id)

   shop:set_name(name)
         :set_options(options)
         :set_shopkeeper_level_range(0, 10)
         :set_inventory_rarity("junk", "common", "uncommon", "rare")
         :set_inventory_max_net_worth(100)
         :stock_shop()

   assert(shop)

   return shop
end

function ShopService:destroy_shop(shop)
   shop:destroy()
end

---Returns a table (keyed by uri) of an instance of every item in the world which is stockable.  These 
-- are all the entities mentioned in the manifest of every mod which have the stonehearth:net_worth 
-- entity data where shop_info.sellable = true.
function ShopService:get_sellable_items()
   return self._sv.sellable_items
end

function ShopService:_init_sellable_items()
   if self._sv.sellable_items ~= nil then
      return
   end

   self._sv.sellable_items = {}

   local mods = radiant.resources.get_mod_list()

   -- for each mod
   for i, mod in ipairs(mods) do
      local manifest = radiant.resources.load_manifest(mod)
      -- for each alias
      if manifest.aliases then
         for alias, uri in pairs(manifest.aliases) do
            -- is it sellable?
            local full_alias = mod .. ':' .. alias
            local path = radiant.resources.convert_to_canonical_path(full_alias)

            if path ~= nil and self:_alias_is_sellable_entity(path) then
               -- add the entity to the table of sellable items
               local entity = radiant.entities.create_entity(path)
               self._sv.sellable_items[full_alias] = entity
            end
         end
      end
   end
   self.__saved_variables:mark_changed()
end

function ShopService:get_entity_value_in_gold(entitiy)
   local value = 0
   local entity_data = radiant.entities.get_entity_data(entity, 'stonehearth:net_worth')
   
   if entity_data ~= nil and entity_data.shop_info ~= nil and entity_data.shop_info.value_in_gold ~= nil then
      value = entity_data.shop_info.value_in_gold
   end

   return value
end

--- Check that this is a valid entity to sell, which means the following:
-- 1. The uri is for an entity and not an action or something else
-- 2. The entity has stonehearth:net_worth entity_data
-- 3. The entity's net worth says it is sellable
function ShopService:_alias_is_sellable_entity(path)

   if string.sub(path, -5) ~= '.json' then
      return false
   end

   local json = radiant.resources.load_json(path)

   if not json.type or json.type ~= 'entity' then
      return false
   end

   local entity_data = json.entity_data
   
   if entity_data == nil or 
      entity_data['stonehearth:net_worth'] == nil or 
      entity_data['stonehearth:net_worth'].value_in_gold <= 0 or
      entity_data['stonehearth:net_worth'].shop_info == nil or
      not entity_data['stonehearth:net_worth'].shop_info.buyable then
      
      return false
   end

   return true
end

return ShopService
