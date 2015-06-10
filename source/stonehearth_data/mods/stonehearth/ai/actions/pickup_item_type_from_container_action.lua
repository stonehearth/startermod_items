local Entity = _radiant.om.Entity
local PickupItemTypeFromContainer = class()

PickupItemTypeFromContainer.name = 'pickup item type from container'
PickupItemTypeFromContainer.does = 'stonehearth:pickup_item_type_from_container'
PickupItemTypeFromContainer.args = {
   backpack_entity = Entity,
   filter_fn = 'function',
   description = 'string',
}
PickupItemTypeFromContainer.think_output = {
   item = Entity,          -- what actually got picked up
}
PickupItemTypeFromContainer.version = 2
PickupItemTypeFromContainer.priority = 1

function PickupItemTypeFromContainer:start_thinking(ai, entity, args)
   if not ai.CURRENT.carrying then
      ai:set_think_output()
   end
end

function PickupItemTypeFromContainer:start(ai, entity, args)
   self._crate_location_trace = radiant.entities.trace_location(args.backpack_entity, 'crate location trace')
      :on_changed(function()
            ai:abort('drop carrying in crate destination moved.')
         end)
end


function PickupItemTypeFromContainer:stop(ai, entity, args)
   if self._crate_location_trace then
      self._crate_location_trace:destroy()
      self._crate_location_trace = nil
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(PickupItemTypeFromContainer)
         :execute('stonehearth:find_backpack_entity_type', {
            filter_fn = ai.ARGS.filter_fn,
            backpack_entity = ai.ARGS.backpack_entity,
         })
         :execute('stonehearth:goto_entity', {
            entity = ai.ARGS.backpack_entity,
         })
         :execute('stonehearth:reserve_entity', { 
            entity = ai.BACK(2).item,
         })
         :execute('stonehearth:pickup_item_from_backpack', { 
            item = ai.BACK(3).item,
            backpack_entity = ai.ARGS.backpack_entity,
         })
         :set_think_output({ 
            item = ai.PREV.item,
         })
         