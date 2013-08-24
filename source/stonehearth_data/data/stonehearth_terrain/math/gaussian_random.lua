-- Polar implementation of the Box-Muller transform
-- Use Ziggurat algorithm if performance is paramount
-- About 3M numbers / second in LUA

-- one standard deviation contains 68.27% of the values
-- two standard deviations contains 95.45% of the values
-- three standard deviations contains 99.73% of the values

local GaussianRandom = class()

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

-- Generates pairs of random numbers in normal distribution centering on 0
-- ~95% of numbers returned should fall between -2 and 2
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

return GaussianRandom
