local Entity = _radiant.om.Entity
local PickupPlacedItemAdjacent = class()

PickupPlacedItemAdjacent.name = 'break down a placed item'
PickupPlacedItemAdjacent.does = 'stonehearth:pickup_item_adjacent'
PickupPlacedItemAdjacent.args = {
   item = Entity     -- the big version
}
PickupPlacedItemAdjacent.version = 2
PickupPlacedItemAdjacent.priority = 2

local log = radiant.log.create_logger('actions.pickup_item')

function PickupPlacedItemAdjacent:start_thinking(ai, entity, args)
   local item = args.item

   if ai.CURRENT.carrying ~= nil then
      return
   end
   self._placed_item_component = item:get_component('stonehearth:entity_forms')
   if self._placed_item_component == nil then
      return
   end
   self._proxy_item = self._placed_item_component:get_proxy_entity()
   
   self._proxy_component = self._proxy_item:add_component('stonehearth:iconic_form')
   
   if not self._proxy_component then
      --TODO: if you just return here, without the assert, the bed will disappear from the world. 
      assert(false, 'the proxy item is invalid. WHY???')
      return
   end
   
   self._proxy_component:set_full_sized_entity(item)

   ai.CURRENT.carrying = self._proxy_item
   ai:set_think_output()
end

function PickupPlacedItemAdjacent:run(ai, entity, args)
   local item = args.item
   
   radiant.entities.turn_to_face(entity, item)
   ai:execute('stonehearth:run_effect', { effect = 'work' })
   
   local location = item:get_component('mob'):get_world_grid_location()   
   radiant.terrain.remove_entity(item)  
   radiant.terrain.place_entity(self._proxy_item, location)
   self._proxy_component:set_full_sized_entity(item)
   
   local pickup_item = self._proxy_item
   self._proxy_item = nil

   --sometimes the location of the parent object is not adjacent to the
   --entity. In that case, walk over to the placed item.
   ai:execute('stonehearth:goto_entity', { entity = pickup_item })
   ai:execute('stonehearth:pickup_item_adjacent', { item = pickup_item })
   self._proxy_item = nil
end

return PickupPlacedItemAdjacent
