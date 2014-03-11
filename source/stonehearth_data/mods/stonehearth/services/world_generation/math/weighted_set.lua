local WeightedSet = class()

function WeightedSet:__init(rng)
   self._rng = rng
   self:clear()
end

function WeightedSet:clear()
   self._map = {}
   self._total_weight = 0
   self._total_is_valid = true
end

function WeightedSet:add(item, weight)
   self._map[item] = weight
   self._total_is_valid = false
end

function WeightedSet:modify(item, weight)
   self:add(item, weight)
end

function WeightedSet:remove(item)
   self:add(item, nil)
end

function WeightedSet:get(item)
   return self._map[item]
end

-- current implementation is O(n)
function WeightedSet:choose_random()
   if not self._total_is_valid then
      self:_calculate_total_weight()
   end

   local roll = self._rng:get_int(1, self._total_weight)
   local sum = 0

   -- traversal order does not matter
   for item, weight in pairs(self._map) do
      sum = sum + weight
      if roll <= sum then
         return item
      end
   end

   -- error: roll > total weight
   assert(false)
end

function WeightedSet:_calculate_total_weight()
   local sum = 0

   for item, weight in pairs(self._map) do
      sum = sum + weight
   end

   self._total_weight = sum
   self._total_is_valid = true
end

return WeightedSet
