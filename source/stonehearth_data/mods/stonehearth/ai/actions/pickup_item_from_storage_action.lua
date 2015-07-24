local Entity = _radiant.om.Entity
local PickupItemFromStorage = class()

PickupItemFromStorage.name = 'pickup item from storage'
PickupItemFromStorage.does = 'stonehearth:pickup_item'
PickupItemFromStorage.args = {
   item = Entity,
}
PickupItemFromStorage.version = 2
PickupItemFromStorage.priority = 1

function PickupItemFromStorage:start_thinking(ai, entity, args)
   if ai.CURRENT.carrying then
      return
   end
   local id = radiant.entities.get_player_id_from_entity(entity)
   local item_entity = radiant.entities.get_iconic_form(args.item)

   if not item_entity then
      item_entity = args.item
   end

   local container = stonehearth.inventory:get_inventory(id):public_container_for(item_entity)
   if container then
      self._storage = container

      ai:set_think_output({
         item = item_entity,
         storage = container,
      })
   end
end

function PickupItemFromStorage:start(ai, entity, args)
   self._storage_location_trace = radiant.entities.trace_location(self._storage, 'storage location trace')
      :on_changed(function()
            ai:abort('storage container moved')
         end)
end


function PickupItemFromStorage:stop(ai, entity, args)
   if self._storage_location_trace then
      self._storage_location_trace:destroy()
      self._storage_location_trace = nil
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(PickupItemFromStorage)
         :execute('stonehearth:goto_entity_in_storage', {
            entity = ai.PREV.item,
         })
         :execute('stonehearth:reserve_entity', { 
            entity = ai.BACK(2).item,
         })
         :execute('stonehearth:pickup_item_from_storage_adjacent', { 
            item = ai.BACK(3).item,
            storage = ai.BACK(3).storage,
         })
         :set_think_output({ 
            item = ai.PREV.item,
         })
         