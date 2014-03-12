local rng = _radiant.csg.get_default_rng()

local LootTableComponent = class()

function LootTableComponent:initialize(entity, json)
   self._entity = entity
   self._loot_table = json
   self._num_shares = 0
   for _, item in ipairs(self._loot_table.items) do
      local frequency 

      if item.frequency then
         frequency = item.frequency
      else
         frequency = 1
      end

      self._num_shares = self._num_shares + frequency
   end
end

-- use this when you want to get the loot and do some operation on the information.
-- If you just want to dump the loot on the ground, use spawn_loot
function LootTableComponent:get_loot()
   local items = {}

   local drops = rng:get_int(self._loot_table.num_drops.min,
                             self._loot_table.num_drops.max)
   
   --[[
   for local i = 1, drops, 1 do
      self:add_loot_item(items)
   end
   ]]

   for _, item in pairs(self._loot_table.items) do
      table.insert(items, item.uri)
   end

   return items
end

function LootTableComponent:spawn_loot()
   local items = self:get_loot()
   local location = radiant.entities.get_location_aligned(self._entity)
   local item_entities = {}
   
   for _, item_uri in ipairs(items) do
      local item = radiant.entities.create_entity(item_uri)
      location.x = location.x + math.random(-1, 1)
      location.z = location.z + math.random(-1, 1)
      radiant.terrain.place_entity(item, location)
      table.insert(item_entities, item)
   end

   return item_entities
end


function LootTableComponent:add_loot_item(items)
   local r = rnd:get_int(0, _num_shares)

end

return LootTableComponent
