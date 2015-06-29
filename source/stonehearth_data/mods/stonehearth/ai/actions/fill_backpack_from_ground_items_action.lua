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
local function pickup_item_off_ground_filter(item, player_id)
   if not item or not item:is_valid() then
      log:spam('%s is not a valid item.  cannot be stored.', tostring(item))
      return false
   end

   if player_id ~= radiant.entities.get_player_id(item) then
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
   local player_id = tostring(radiant.entities.get_player_id(entity))
   local key = 'player_id+' .. player_id
   local filter_fn = ALL_FILTER_FNS[key]
   if not filter_fn then
      filter_fn = function(item)
         return pickup_item_off_ground_filter(item, player_id)
      end
      ALL_FILTER_FNS[key] = filter_fn
   end
   ai:set_think_output({
         filter_fn = filter_fn,
         description = args.description,
      })
end   

function FillBackpackFromGroundItems:start(ai, entity, args)
   self._inventory = stonehearth.inventory:get_inventory(entity)
   if self._inventory then
      self._ai = ai
      self._listener = radiant.events.listen(self._inventory, 'stonehearth:inventory:public_storage_full_changed', self, self._on_space_changed)
   end
end

function FillBackpackFromGroundItems:_on_space_changed()
   local full = self._inventory:public_storage_is_full()
   if full then
      self._ai:abort('out of storage space')
   end
end

function FillBackpackFromGroundItems:stop(ai, entity, args)
   if self._listener then
      self._listener:destroy()
      self._listener = nil
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(FillBackpackFromGroundItems)
   :execute('stonehearth:wait_for_inventory_storage_space')
   :execute('stonehearth:pickup_item_type', {
            filter_fn = ai.BACK(2).filter_fn,
            description = 'fill backpack from ground item',
   })
   :execute('stonehearth:put_carrying_in_backpack', {})
