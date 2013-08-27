local GaussianRandom = radiant.mods.require('/stonehearth_terrain/math/gaussian_random.lua')

local InverseGaussianRandom = class()

-- Inverse Gaussian distribution which is high at the endpoints and low in the middle
-- Splits a Gaussian distribution into two pieces and slides each half to the min and max endpoints
function InverseGaussianRandom.generate(min, max, std_dev)
   local half_range = (max-min)*0.5
   local rand

   while true do
      rand = GaussianRandom.generate(0, std_dev)
      -- truncate the distribution so they don't overlap
      if math.abs(rand) < half_range then break end

      -- extremely rare, but don't double the pdf of the midpoint by including the equality above
      if rand == half_range then break end
   end

   if rand > 0 then
      return min+rand
   else
      if rand < 0 then
         return max+rand
      else
         -- rand == 0, very rare
         local toss = math.random(0, 1)
         if toss == 0 then
            return min
         else
            return max
         end
      end
   end
end

return InverseGaussianRandom
