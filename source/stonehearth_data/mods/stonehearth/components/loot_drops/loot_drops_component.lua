local LootTable = require 'stonehearth.lib.loot_table.loot_table'

--[[
   Used to determine what loot a mob drops when it dies.  This program is for
   dynamicly spcifing loot drops on existing entities!  If you want an entity
   to always
]]

local LootDropsComponent = class()

function LootDropsComponent:initialize(entity, json)
   self._entity = entity
   assert(not next(json), 'use the "stonehearth:destroyed_loot_table" entity data for statically configured loot drops')
end

function LootDropsComponent:destroy()
   local loot_table = self._sv.loot_table
   if loot_table then
      local location = radiant.entities.get_world_grid_location(self._entity)
      if location then
         local items = LootTable(loot_table)
                           :roll_loot()
         radiant.entities.spawn_items(items, location, 1, 3, { owner = self._entity })
      end
   end
end

function LootDropsComponent:set_loot_table(loot_table)
   self._sv.loot_table = loot_table
   self.__saved_variables:mark_changed()
end
return LootDropsComponent
