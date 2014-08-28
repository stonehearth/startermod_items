local Entity = _radiant.om.Entity
local PickupItemTrivial = class()

PickupItemTrivial.name = 'pickup already carried item'
PickupItemTrivial.does = 'stonehearth:pickup_item'
PickupItemTrivial.args = {
   item = Entity,
}
PickupItemTrivial.version = 2
PickupItemTrivial.priority = 1

function PickupItemTrivial:start_thinking(ai, entity, args)
   if ai.CURRENT.carrying == args.item then
      ai:set_think_output()
   end
end

return PickupItemTrivial
