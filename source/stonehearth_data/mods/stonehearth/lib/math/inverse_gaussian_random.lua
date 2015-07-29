local InverseGaussianRandom = class()
local log = radiant.log.create_logger('math')

function InverseGaussianRandom:__init(rng)
   self._rng = rng
end

-- Inverse Gaussian distribution which is high at the endpoints and low in the middle
-- Splits a Gaussian distribution into two pieces and slides each half to the min and max endpoints
function InverseGaussianRandom:get_real(min, max, std_dev)
   local rng = self._rng
   local half_range = (max-min)*0.5
   local rand

   while true do
      rand = rng:get_gaussian(0, std_dev)
      -- truncate the distribution so they don't overlap
      if math.abs(rand) < half_range then break end

      -- extremely rare, but don't double the pdf of the midpoint by including in the equality above
      if rand == half_range then break end
   end

   if rand > 0 then
      return min+rand
   elseif rand < 0 then
      return max+rand
   else -- rand == 0, very rare
      local toss = rng:get_int(0, 1)
      if toss == 0 then
         return min
      else
         return max
      end
   end
end

function InverseGaussianRandom:get_int(min, max, std_dev)
   -- extend range by 0.5 on each end so that endpoint distribution is not clipped
   local rand_float = self:get_real(min-0.5, max+0.5, std_dev)
   local rand_int = radiant.math.round(rand_float)

   -- the extremely rare case where rand_float == max
   --    and +0.5 rounding truncates to the next higher integer
   if rand_int > max then
      return max
   end

   return rand_int
end

function InverseGaussianRandom:simulate_probabilities(min, max, std_dev, iterations)
   local counts = {}
   local probabilities = {}
   local i, rand

   for i=min, max do
      counts[i] = 0
   end

   for i=1, iterations do
      rand = self:get_int(min, max, std_dev)
      counts[rand] = counts[rand] + 1
   end

   for i=min, max do
      probabilities[i] = counts[i] / iterations
   end

   return probabilities
end

function InverseGaussianRandom:print_probabilities(min, max, std_dev, iterations)
   local i, probabilities
   probabilities = self:simulate_probabilities(min, max, std_dev, iterations)

   for i=min, max do
      log:info('%d: %.2f%%', i, probabilities[i]*100)
   end
end

return InverseGaussianRandom