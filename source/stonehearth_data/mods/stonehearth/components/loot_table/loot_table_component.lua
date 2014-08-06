local WeightedSet = require 'services.server.world_generation.math.weighted_set'
local rng = _radiant.csg.get_default_rng()

local LootTableComponent = class()

function LootTableComponent:initialize(entity, json)
   self._entity = entity
   self._num_drops = json.num_drops

   local loot_table = WeightedSet(rng)

   for _, loot_info in pairs(json.items) do
      local frequency = loot_info.frequency or 1
      loot_table:add(loot_info.uri, frequency)
   end

   self._loot_table = loot_table
end

-- use this when you want to get the loot and do some operation on the information.
-- If you just want to dump the loot on the ground, use spawn_loot
function LootTableComponent:roll_loot()
   local item_uris = {}
   local num_drops = rng:get_int(self._num_drops.min, self._num_drops.max)
   
   for i = 1, num_drops do
      local item_uri = self._loot_table:choose_random()
      table.insert(item_uris, item_uri)
   end

   return item_uris
end

function LootTableComponent:spawn_loot()
   local origin = radiant.entities.get_world_grid_location(self._entity)
   local items_uris = self:roll_loot()
   local items = {}
   
   for _, item_uri in pairs(items_uris) do
      local item = radiant.entities.create_entity(item_uri)
      local location = radiant.terrain.find_placement_point(origin, 1, 2)
      radiant.terrain.place_entity(item, location)
      items[item:get_id()] = item
   end

   return items
end

return LootTableComponent
