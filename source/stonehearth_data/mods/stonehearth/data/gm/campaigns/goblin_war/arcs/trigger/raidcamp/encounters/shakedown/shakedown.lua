local rng = _radiant.csg.get_default_rng()

local ShakeDown = class()

function ShakeDown:initialize(ctx)
   self._sv.ctx = ctx
   self._sv.max_tries = 4
   self._sv.tribute_value = 100
   self:_construct()
end

function ShakeDown:restore()
   self:_construct()
end

-- cache the information for the object we'll use to fill up to
-- the needed tribute value
--
function ShakeDown:_construct()
   local uri = 'stonehearth:resources:wood:oak_log'
   local info = radiant.resources.load_json(uri)
   self._filler_info = {
      uri = uri,
      icon = info.components.unit_info.icon,
      display_name = info.components.unit_info.name,
      worth = self:_get_value_in_gold(uri),
   }
end

-- return a table containing the tribute information needed for the
-- demand tribute encounter
--
function ShakeDown:get_tribute_demand()
   local ctx = self._sv.ctx
   local player_id = ctx.player_id

   local inventory = stonehearth.inventory:get_inventory(player_id)
   if not inventory then
      return
   end
   
   local items = inventory:get_item_tracker('stonehearth:basic_inventory_tracker')
                              :get_tracking_data()
   local keys = radiant.keys(items)
   local key_count = #keys

   local tries = 0
   local max_tries = self._sv.max_tries
   local remaining_value = self._sv.tribute_value
   local tribute = {}

   while key_count > 0 and tries < max_tries and remaining_value > 0 do
      local uri = keys[rng:get_int(1, key_count)]
      local worth = self:_get_value_in_gold(uri)
      if worth then
         local cap = (remaining_value / worth) + 1
         local count = rng:get_int(1, cap)
         if not tribute[uri] then
            local info = items[uri]
            tribute[uri] = {
               uri = uri,
               count = 0, 
               icon = info.icon,
               display_name = info.display_name,
            }
         end
         tribute[uri].count = tribute[uri].count + count
         remaining_value = remaining_value - (count * worth)
      end
      tries = tries + 1
   end

   -- make up the rest in oak logs
   if remaining_value > 0 then
      local fi = self._filler_info
      local uri = fi.uri

      local count = remaining_value / fi.worth
      if not tribute[uri] then
         tribute[uri] = {
            uri = uri,
            count = 0, 
            icon = fi.icon,
            display_name = fi.display_name,
         }
      end
      tribute[uri].count = tribute[uri].count + count
   end

   -- the price of poker just went up!
   self._sv.tribute_value = self._sv.tribute_value * 1.25;

   return tribute
end

function ShakeDown:_get_value_in_gold(uri)
   local net_worth = radiant.entities.get_entity_data(uri, 'stonehearth:net_worth')
   return net_worth and net_worth.value_in_gold or 0
end

return ShakeDown
