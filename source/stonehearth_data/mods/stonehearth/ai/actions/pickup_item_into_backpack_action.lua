local Entity = _radiant.om.Entity
local PickupItemIntoBackpack = class()

PickupItemIntoBackpack.name = 'pick up item into backpack'
PickupItemIntoBackpack.does = 'stonehearth:pickup_item_into_backpack'
PickupItemIntoBackpack.args = {
   item = Entity,
}
PickupItemIntoBackpack.version = 2
PickupItemIntoBackpack.priority = 1

function PickupItemIntoBackpack:start_thinking(ai, entity, args)
   local sc = entity:get_component('stonehearth:storage')
   if sc and not sc:is_full() then
      ai:set_think_output()
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(PickupItemIntoBackpack)
   :execute('stonehearth:pickup_item', { item = ai.ARGS.item })
   :execute('stonehearth:put_carrying_in_backpack', {})
