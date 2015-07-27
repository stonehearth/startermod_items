local IntegerGaussianRandom = class()
local log = radiant.log.create_logger('math')

function IntegerGaussianRandom:__init(rng)
   self._rng = rng
   if not self._rng then
      self._rng = _radiant.csg.get_default_rng()
   end
end

function IntegerGaussianRandom:get_int(min, max, std_dev)
   local rng = self._rng
   local mean = (min+max)*0.5
   local min_float = min-0.5
   local max_float = max+0.5
   local rand

   for i=1,10 do
      rand = rng:get_gaussian(mean, std_dev)

      -- do not break when rand == max_float because of 0.5 rounding
      if rand >= min_float and rand < max_float then
         return radiant.math.round(rand)
      end
   end

   -- failed?  the std_dev must be MASSIVE.  resort to linear.
   log:detail('falling back to lineary distribution for gaussian random int between %d and %d (stddev:%.2f)', min, max, std_dev)
   return rng:get_int(min, max)
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
