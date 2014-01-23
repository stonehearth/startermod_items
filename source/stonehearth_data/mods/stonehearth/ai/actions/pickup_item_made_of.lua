local Entity = _radiant.om.Entity
local PickupItemMadeOf = class()

PickupItemMadeOf.name = 'pickup item made of'
PickupItemMadeOf.does = 'stonehearth:pickup_item_made_of'
PickupItemMadeOf.args = {
   material = 'string',      -- the material tags we need
}
PickupItemMadeOf.think_output = {
   item = Entity,            -- what was actually picked up
}
PickupItemMadeOf.version = 2
PickupItemMadeOf.priority = 1

local is_material = function (material, item)
   return radiant.entities.is_material(item, material)
end

local ai = stonehearth.ai
return ai:create_compound_action(PickupItemMadeOf)
         :execute('stonehearth:pickup_item_type', {
            filter_fn = ai:create_function(is_material, ai.ARGS.material)
         })            
         :set_think_output({ item = ai.PREV.item })
