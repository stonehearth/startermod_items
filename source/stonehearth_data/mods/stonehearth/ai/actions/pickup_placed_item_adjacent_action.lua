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
   self._placed_item_component = item:get_component('stonehearth:placed_item')
   if self._placed_item_component == nil then
      return
   end
   self._proxy_item = self._placed_item_component:get_proxy_entity()
   self._proxy_component = self._proxy_item:add_component('stonehearth:placeable_item_proxy')
   
   if not self._proxy_item or not self._proxy_item then
      assert(false, 'Huh? Why would either be nil?')
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
   radiant.terrain.place_entity(self._proxy_item, location)
   radiant.terrain.remove_entity(item)
   self._proxy_component:set_full_sized_entity(item)
   
   local pickup_item = self._proxy_item
   self._proxy_item = nil

   --sometimes the location of the parent object is not adjacent to the
   --entity. In that case, walk over to the placed item.
   ai:execute('stonehearth:goto_entity', { entity = pickup_item })
   ai:execute('stonehearth:pickup_item_adjacent', { item = pickup_item })
   self._proxy_item = nil
end

function PickupPlacedItemAdjacent:stop()
   if self._proxy_item then
      radiant.entities.destroy_entity(self._proxy_item)
      self._proxy_item = nil
   end
end

return PickupPlacedItemAdjacent
