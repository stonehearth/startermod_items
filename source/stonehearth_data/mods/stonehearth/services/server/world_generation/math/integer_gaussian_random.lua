local IntegerGaussianRandom = class()
local log = radiant.log.create_logger('world_generation')

function IntegerGaussianRandom:__init(rng)
   self._rng = rng
end

function IntegerGaussianRandom:get_int(min, max, std_dev)
   local rng = self._rng
   local mean = (min+max)*0.5
   local min_float = min-0.5
   local max_float = max+0.5
   local rand

   while true do
      rand = rng:get_gaussian(mean, std_dev)

      -- do not break when rand == max_float because of 0.5 rounding
      if rand >= min_float and rand < max_float then break end
   end

   return radiant.math.round(rand)
end

function IntegerGaussianRandom:simulate_probabilities(min, max, std_dev, iterations)
   local counts = {}
   local probabilities = {}
   local i, rand

   for i=min, max do
      counts[i] = 0
   end

   for i=1, iterations do
      rand = IntegerGaussianRandom.get_int(min, max, std_dev)
      counts[rand] = counts[rand] + 1
   end

   for i=min, max do
      probabilities[i] = counts[i] / iterations
   end

   return probabilities
end

function IntegerGaussianRandom:print_probabilities(min, max, std_dev, iterations)
   local i, probabilities
   probabilities = IntegerGaussianRandom.simulate_probabilities(min, max, std_dev, iterations)

   for i=min, max do
      log:info('', '%d: %.2f%%', i, probabilities[i]*100)
   end
end

return IntegerGaussianRandom
