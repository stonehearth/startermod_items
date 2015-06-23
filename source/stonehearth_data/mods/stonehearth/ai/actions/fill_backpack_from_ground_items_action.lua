local Entity = _radiant.om.Entity
local FillBackpackFromGroundItems = class()

FillBackpackFromGroundItems.name = 'fill the backpack with ground items'
FillBackpackFromGroundItems.does = 'stonehearth:simple_labor'
FillBackpackFromGroundItems.args = {}
FillBackpackFromGroundItems.version = 2
FillBackpackFromGroundItems.priority = stonehearth.constants.priorities.simple_labor.RESTOCK_STOCKPILE

local log = radiant.log.create_logger('storage')

-- xxx share this logic with storage component (move shared bits into storage_lib or something)
--
local function pickup_item_off_ground_filter(item)
   if not item or not item:is_valid() then
      log:spam('%s is not a valid item.  cannot be stored.', tostring(item))
      return false
   end

   local efc = item:get_component('stonehearth:entity_forms')
   if efc and efc:get_should_restock() then
      return true
   end

   if not item:get_component('item') then
      log:spam('%s is not an item material.  cannot be stored.', item)
      return false
   end

   local material = item:get_component('stonehearth:material')
   if not material then
      log:spam('%s has no material.  cannot be stored.', item)
      return false
   end

   local unit_info = item:get_component('unit_info')
   if not unit_info then
      return false
   end
   local player_id = unit_info:get_player_id()
   if not player_id then
      return false
   end
   local inventory = stonehearth.inventory:get_inventory(player_id)
   if not inventory then
      return false
   end
   local storage = inventory:container_for(item)
   if storage then
      local passes_filter = storage:get_component('stonehearth:storage')
                                       :passes(item)
      if passes_filter then
         return false
      end
   end

   return true
end

function FillBackpackFromGroundItems:start_thinking(ai, entity, args)
   ai:set_think_output()
end

local ai = stonehearth.ai
return ai:create_compound_action(FillBackpackFromGroundItems)
   :execute('stonehearth:pickup_item_type', {
            filter_fn = pickup_item_off_ground_filter,
            description = 'fill backpack from ground item',
   })
   :execute('stonehearth:put_carrying_in_backpack', {})
