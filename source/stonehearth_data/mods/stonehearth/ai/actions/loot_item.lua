local Entity = _radiant.om.Entity
local LootItem = class()

LootItem.name = 'loot item'
LootItem.does = 'stonehearth:loot_item'
LootItem.args = {
   item = Entity,
}
LootItem.version = 2
LootItem.priority = 1

function LootItem:start_thinking(ai, entity, args)
   local inventory_component = entity:add_component('stonehearth:inventory')
   if inventory_component and not inventory_component:is_full() then
      ai:set_think_output()
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(LootItem)
   :execute('stonehearth:pickup_item', { item = ai.ARGS.item })
   :execute('stonehearth:put_carrying_in_inventory')
