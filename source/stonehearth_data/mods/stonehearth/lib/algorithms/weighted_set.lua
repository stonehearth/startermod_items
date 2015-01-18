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
   assert(weight >= 0)
   self._map[item] = weight
   self._total_is_valid = false
end

function WeightedSet:modify(item, weight)
   self:add(item, weight)
end

function WeightedSet:remove(item)
   self:add(item, nil)
end

function WeightedSet:get_weight(item)
   return self._map[item]
end

function WeightedSet:get_probability(item)
   self:_calculate_total_weight()

   local item_weight = self._map[item]
   return item_weight / self._total_weight
end

-- current implementation is O(n)
function WeightedSet:choose_random()
   self:_calculate_total_weight()

   if self._total_weight == 0 then
      return nil
   end

   local roll = self._rng:get_real(0, self._total_weight)
   local sum = 0
   local item, weight

   -- traversal order does not matter
   for item, weight in pairs(self._map) do
      sum = sum + weight

      -- needs to be < not <=
      if roll < sum then
         return item
      end
   end

   -- should only get here via cumulative rounding error
   local epsilon = self._total_weight / 1000000000
   assert(roll < self._total_weight + epsilon)

   -- return the last item, which is what should have been returned without a rounding error
   return item
end

function WeightedSet:_calculate_total_weight()
   if self._total_is_valid then
      return
   end

   local sum = 0

   for item, weight in pairs(self._map) do
      sum = sum + weight
   end

   self._total_weight = sum
   self._total_is_valid = true
end

return WeightedSet
