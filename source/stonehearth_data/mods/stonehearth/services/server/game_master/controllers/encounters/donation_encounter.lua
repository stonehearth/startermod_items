local Entity = _radiant.om.Entity
local rng = _radiant.csg.get_default_rng()
local LootTable = require 'stonehearth.lib.loot_table.loot_table'

--Takes a format like a loot table.
--Selects a number of items and drops them by the player's camp standard

local DonationEncounter = class()

function DonationEncounter:activate()
   self._log = radiant.log.create_logger('game_master.encounters.donation_encounter')
end

function DonationEncounter:start(ctx, info)
   
   --Get the drop location
   assert(ctx.player_id, "We don't have a player_id for this player")
   local town = stonehearth.town:get_town(ctx.player_id)
   local banner = town:get_banner()
   local drop_origin = banner and radiant.entities.get_world_grid_location(banner)
   if not drop_origin then
      return
   end

   --Get the drop items
   assert(info.loot_table)
   local items = LootTable(info.loot_table)
                     :roll_loot()
   local spawned_entities = radiant.entities.spawn_items(items, drop_origin, 1, 3, { owner = ctx.player_id })

   ctx.arc:trigger_next_encounter(ctx)
end

function DonationEncounter:stop()
end

return DonationEncounter