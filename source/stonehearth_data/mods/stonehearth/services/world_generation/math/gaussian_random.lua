-- Polar implementation of the Box-Muller transform
-- Use Ziggurat algorithm if performance is paramount
-- About 3M numbers / second in LUA

-- one standard deviation contains 68.27% of the values
-- two standard deviations contains 95.45% of the values
-- three standard deviations contains 99.73% of the values

local MathFns = require 'services.world_generation.math.math_fns'

local GaussianRandom = class()
local log = radiant.log.create_logger('world.generation')

local next_random = nil

function GaussianRandom.generate(mean, std_dev)
   local current_random

   current_random = next_random

   if current_random then
      next_random = nil
   else
      current_random, next_random = GaussianRandom._generate_pair()
   end

   return mean + current_random*std_dev
end

function GaussianRandom.generate_int(min, max, std_dev)
   local mean = (min+max)*0.5
   local min_float = min-0.5
   local max_float = max+0.5
   local rand

   while true do
      rand = GaussianRandom.generate(mean, std_dev)

      -- do not break when rand == max_float because of 0.5 rounding
      if rand >= min_float and rand < max_float then break end
   end

   return MathFns.round(rand)
end

-- Generates pairs of random numbers in normal distribution with a
--   mean of 0 and standard deviation of 1
function GaussianRandom._generate_pair()
   local u, v, r, c

   while true do
      -- generate points uniformly distributed in the (-1,-1) to (1,1) square
      u = 2*math.random()-1
      v = 2*math.random()-1

      -- calculate the length (squared) of the vector
      r = u*u + v*v

      -- only accept points (u,v) that lie inside the unit circle (except 0,0)
      if (r <= 1) and (r > 0) then break end
   end

   c = math.sqrt(-2*math.log(r)/r)

   return u*c, v*c
end

function GaussianRandom.simulate_probabilities(min, max, std_dev, iterations)
   local counts = {}
   local probabilities = {}
   local i, rand

   for i=min, max do
      counts[i] = 0
   end

   for i=1, iterations do
      rand = GaussianRandom.generate_int(min, max, std_dev)
      counts[rand] = counts[rand] + 1
   end

   for i=min, max do
      probabilities[i] = counts[i] / iterations
   end

   return probabilities
end

function GaussianRandom.print_probabilities(min, max, std_dev, iterations)
   local i, probabilities
   probabilities = GaussianRandom.simulate_probabilities(min, max, std_dev, iterations)

   for i=min, max do
      log:info('', '%d: %.2f%%', i, probabilities[i]*100)
   end
end

return GaussianRandom
