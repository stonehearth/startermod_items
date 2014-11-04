local WeightedSet = require 'lib.algorithms.weighted_set'
local rng = _radiant.csg.get_default_rng()
local log = radiant.log.create_logger('loot_table')

local LootTable = class()

function LootTable:__init()
   self:_clear()
end

-- Entity may be a uri or valid entity
function LootTable:load_from_entity(entity)
   local json = radiant.entities.get_entity_data(entity, 'stonehearth:loot_table')
   self:load_from_json(json)
end

-- Sample json:
-- {
--    "num_rolls" : {
--       "min" : 2,
--       "max" : 4
--    },
--    "items" : [
--       { "uri" : "stonehearth:silver_coin", "weight" : 10 },
--       { "uri" : "stonehearth:gold_coin",   "weight" :  1 }
--    ]
-- }
--
-- "num_rolls":
--    a) Can be a table with keys "min" and "max"
--    b) Can be a number
--    c) Can be omitted (which will default to 1)
--
-- "items":
--    a) Array of entries with keys "uri" and "weight"
--    b) If "uri" is the empty string, no drop is created if it is rolled
--    c) If "weight" is omitted, defaults to 1. Non-integer values are ok.
function LootTable:load_from_json(json)
   if not json then
      self:_clear()
      return
   end

   local num_rolls = json.num_rolls

   if not num_rolls then
      self._min_rolls = 1
      self._max_rolls = 1
   elseif type(num_rolls) == 'number' then
      self._min_rolls = num_rolls
      self._max_rolls = num_rolls
   elseif type(num_rolls) == 'table' then
      self._min_rolls = num_rolls.min
      self._max_rolls = num_rolls.max
   else
      log:error('illegal value for num_rolls: %s', num_rolls)
      self:_clear()
      return
   end

   self._items = WeightedSet(rng)

   for _, entry in pairs(json.items) do
      if entry.uri then
         self._items:add(entry.uri, entry.weight or 1)
      else
         log:error('no uri')
      end
   end
end

-- Returns a table of uri, quantity
function LootTable:roll_loot()
   local uris = {}
   local num_rolls = rng:get_int(self._min_rolls, self._max_rolls)
   
   for i = 1, num_rolls do
      local uri = self._items:choose_random()
      if uri and uri ~= '' then
         local quantity = (uris[uri] or 0) + 1
         uris[uri] = quantity
      end
   end

   return uris
end

function LootTable:_clear()
   self._items = nil
   self._min_rolls = 0
   self._max_rolls = 0
end

return LootTable
