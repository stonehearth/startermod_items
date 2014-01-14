local Entity = _radiant.om.Entity
local PickupItemMadeOf = class()

PickupItemMadeOf.name = 'pickup item made of'
PickupItemMadeOf.does = 'stonehearth:pickup_item_made_of'
PickupItemMadeOf.args = {
   material = 'string',      -- the material tags we need
}
PickupItemMadeOf.version = 2
PickupItemMadeOf.priority = 1

function PickupItemMadeOf:transform_arguments(ai, entity, args)
   return {
      filter_fn = function(item)
            return radiant.entities.is_material(item, args.material)
         end
   }
end

local ai = stonehearth.ai
return ai:create_compound_action(PickupItemMadeOf)
         :execute('stonehearth:pickup_item_type', { filter_fn = ai.XFORMED_ARG.filter_fn })
