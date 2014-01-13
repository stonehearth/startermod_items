local Entity = _radiant.om.Entity
local PickupMaterial = class()

PickupMaterial.name = 'pickup a material'
PickupMaterial.does = 'stonehearth:pickup_material'
PickupMaterial.args = {
   material = 'string',      -- the material tags we need
}
PickupMaterial.version = 2
PickupMaterial.priority = 1

function PickupMaterial:eval_arguments(ai, entity, args)
   return {
      filter_fn = function(item)
            return radiant.entities.is_material(item, args.material)
         end
   }
end

local ai = stonehearth.ai
return ai:create_compound_action(PickupMaterial)
         :execute('stonehearth:pickup_item_type', { filter_fn = ai.ARGS.filter_fn })
