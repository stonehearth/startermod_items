local LootTable = require 'stonehearth.lib.loot_table.loot_table'

--[[
   Used to determine what loot a mob drops when it dies.  This program is for
   dynamicly spcifing loot drops on existing entities!  If you want to declare loot
   constantly, then use the stonehearth:destroyed_loot_table entity data.
]]

local LootDropsComponent = class()

function LootDropsComponent:initialize(entity, json)
   self._entity = entity
   assert(not next(json), 'use the "stonehearth:destroyed_loot_table" entity data for statically configured loot drops')
   self._kill_listener = radiant.events.listen(entity, 'stonehearth:kill_event', self, self._on_kill_event)
end

--There's a difference between destroy and kill; only drop loot on kill. Otherwise,
--the entity drops loot even when the DM just removes him from the game
function LootDropsComponent:_on_kill_event()
   local loot_table = self._sv.loot_table
   if loot_table then
      local location = radiant.entities.get_world_grid_location(self._entity)
      if location then
         local items = LootTable(loot_table)
                           :roll_loot()
         local spawned_entities = radiant.entities.spawn_items(items, location, 1, 3, { owner = self._entity })

         --Add a loot command to each of the spawned items
         for id, entity in pairs(spawned_entities) do 
            local target = entity
            local entity_forms = entity:get_component('stonehearth:entity_forms')
            if entity_forms then
               target = entity_forms:get_iconic_entity()
            end
            local command_component = target:add_component('stonehearth:commands')
            command_component:add_command('/stonehearth/data/commands/loot_item')
         end
      end
   end
end

--clean up the listeners
function LootDropsComponent:destroy()
   self._kill_listener:destroy()
   self._kill_listener = nil
end

function LootDropsComponent:set_loot_table(loot_table)
   self._sv.loot_table = loot_table
   self.__saved_variables:mark_changed()
end
return LootDropsComponent
