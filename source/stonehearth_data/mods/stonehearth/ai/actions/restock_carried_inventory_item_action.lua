local Entity = _radiant.om.Entity
local RestockItemFromBackpack = class()

RestockItemFromBackpack.name = 'restock item from backpack'
RestockItemFromBackpack.does = 'stonehearth:restock_backpack_item'
RestockItemFromBackpack.args = {
   item = Entity,
}
RestockItemFromBackpack.version = 2
RestockItemFromBackpack.priority = 1

function RestockItemFromBackpack:start_thinking(ai, entity, args)
end

function RestockItemFromBackpack:run(ai, entity, args)
end

return RestockItemFromBackpack
