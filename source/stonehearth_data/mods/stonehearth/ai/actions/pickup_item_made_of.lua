local Entity = _radiant.om.Entity
local PickupItemMadeOf = class()

PickupItemMadeOf.name = 'pickup item made of'
PickupItemMadeOf.does = 'stonehearth:pickup_item_made_of'
PickupItemMadeOf.args = {
   material = 'string',      -- the material tags we need
   reconsider_event_name = {
      type = 'string',
      default = '',
   },
}
PickupItemMadeOf.think_output = {
   item = Entity,            -- what was actually picked up
}
PickupItemMadeOf.version = 2
PickupItemMadeOf.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(PickupItemMadeOf)
         :execute('stonehearth:material_to_filter_fn', {
            material = ai.ARGS.material
         })
         :execute('stonehearth:pickup_item_type', {
            filter_fn = ai.PREV.filter_fn,
            description = ai.ARGS.material,
            reconsider_event_name = ai.ARGS.reconsider_event_name,
         })
         :set_think_output({ item = ai.PREV.item })
