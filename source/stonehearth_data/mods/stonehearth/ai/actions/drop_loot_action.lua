local DropLootAction = class()
local LootTable = require 'lib.loot_table.loot_table'

DropLootAction.name = 'drop loot action'
DropLootAction.does = 'stonehearth:drop_loot_action'
DropLootAction.args = {}
DropLootAction.version = 2
DropLootAction.priority = 1

function DropLootAction:run(ai, entity, args)
   local location = radiant.entities.get_world_grid_location(entity)
   local loot_table = LootTable()
   loot_table:load_from_entity(entity)
   local uris = loot_table:roll_loot()
   local items = radiant.entities.spawn_items(uris, location, 1, 3)

end

return DropLootAction
