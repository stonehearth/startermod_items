local Entity = _radiant.om.Entity
local PickupItemMadeOfTrivial = class()

PickupItemMadeOfTrivial.name = 'pickup item made of trivial'
PickupItemMadeOfTrivial.does = 'stonehearth:pickup_item_made_of'
PickupItemMadeOfTrivial.args = {
   material = 'string',      -- the material tags we need
   reconsider_event_name = {
      type = 'string',
      default = '',
   },   
}
PickupItemMadeOfTrivial.think_output = {
   item = Entity
}
PickupItemMadeOfTrivial.version = 2
PickupItemMadeOfTrivial.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(PickupItemMadeOfTrivial)
         :when( function (ai, entity, args)
               return radiant.entities.is_material(ai.CURRENT.carrying, args.material)
            end )
         :set_think_output({ item = ai.CURRENT.carrying })
