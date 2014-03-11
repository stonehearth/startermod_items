local Entity = _radiant.om.Entity
local LootItem = class()

LootItem.name = 'loot an item'
LootItem.does = 'stonehearth:loot_item'
LootItem.args = {
   item = Entity,
}
LootItem.version = 2
LootItem.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(LootItem)
         :when( function (ai, entity, args)
               local inventory_component = entity:add_component('stonehearth:inventory')
               if inventory_component ~= nil then
                  return not inventory_component:is_full();
               end
               
               return false;
            end )
         :execute('stonehearth:pickup_item', { item = ai.ARGS.item })
         :execute('stonehearth:put_carrying_in_inventory', {})
