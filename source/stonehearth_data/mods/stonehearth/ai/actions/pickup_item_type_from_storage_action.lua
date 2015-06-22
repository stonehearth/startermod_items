local Entity = _radiant.om.Entity
local PickupItemTypeFromStorage = class()

PickupItemTypeFromStorage.name = 'pickup item type from storage'
PickupItemTypeFromStorage.does = 'stonehearth:pickup_item_type_from_storage'
PickupItemTypeFromStorage.args = {
   storage = Entity,
   filter_fn = 'function',
   description = 'string',
}
PickupItemTypeFromStorage.think_output = {
   item = Entity,          -- what actually got picked up
}
PickupItemTypeFromStorage.version = 2
PickupItemTypeFromStorage.priority = 1

function PickupItemTypeFromStorage:start_thinking(ai, entity, args)
   if not ai.CURRENT.carrying then
      ai:set_think_output()
   end
end

function PickupItemTypeFromStorage:start(ai, entity, args)
   self._crate_location_trace = radiant.entities.trace_location(args.storage, 'crate location trace')
      :on_changed(function()
            ai:abort('drop carrying in crate destination moved.')
         end)
end


function PickupItemTypeFromStorage:stop(ai, entity, args)
   if self._crate_location_trace then
      self._crate_location_trace:destroy()
      self._crate_location_trace = nil
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(PickupItemTypeFromStorage)
         :execute('stonehearth:find_entity_type_in_storage', {
            filter_fn = ai.ARGS.filter_fn,
            storage = ai.ARGS.storage,
         })
         :execute('stonehearth:goto_entity', {
            entity = ai.ARGS.storage,
         })
         :execute('stonehearth:reserve_entity', { 
            entity = ai.BACK(2).item,
         })
         :execute('stonehearth:pickup_item_from_storage_adjacent', { 
            item = ai.BACK(3).item,
            storage = ai.ARGS.storage,
         })
         :set_think_output({ 
            item = ai.PREV.item,
         })
         