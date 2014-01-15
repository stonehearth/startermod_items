local Entity = _radiant.om.Entity
local PickupItemTrivial = class()

PickupItemTrivial.name = 'pickup already carried item'
PickupItemTrivial.does = 'stonehearth:pickup_item'
PickupItemTrivial.args = {
   item = Entity,
}
PickupItemTrivial.version = 2
PickupItemTrivial.priority = 1


local ai = stonehearth.ai
return ai:create_compound_action(PickupItemTrivial)
         :when( function (ai)
               return ai.CURRENT.carrying == args.item
            end )
