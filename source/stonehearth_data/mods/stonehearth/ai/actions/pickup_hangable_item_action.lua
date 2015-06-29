local Entity = _radiant.om.Entity
local PickupHangableItem = class()

PickupHangableItem.name = 'pickup hangable item'
PickupHangableItem.does = 'stonehearth:pickup_item'
PickupHangableItem.args = {
   item = Entity,
}
PickupHangableItem.version = 2
PickupHangableItem.priority = 2

function PickupHangableItem:start_thinking(ai, entity, args)
   ai:monitor_carrying()
   if ai.CURRENT.carrying then
      return
   end

   local entity_forms_component = args.item:get_component('stonehearth:entity_forms')
   if entity_forms_component and entity_forms_component:is_placeable_on_wall() then
      ai:set_think_output()
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(PickupHangableItem)
         :execute('stonehearth:reserve_entity', {
               entity = ai.ARGS.item
            })
         :execute('stonehearth:get_entity_location', {
               entity = ai.ARGS.item
            })
         :execute('stonehearth:create_entity', {
               location = ai.PREV.grid_location,
               options = {
                  allow_vertical_adjacent = true,
                  debug_text = 'pick up hangable item',
               }
            })
         :execute('stonehearth:goto_entity', {
               entity = ai.PREV.entity
            })
         :execute('stonehearth:pickup_item_adjacent', {
               item = ai.ARGS.item
            })
