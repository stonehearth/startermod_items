-- See notes on GaussianNoise
local InverseGaussianNoise = class()
local GaussianNoise = radiant.mods.require('/stonehearth_terrain/gaussian_noise.lua')

function InverseGaussianNoise:__init(amplitude, quality)
   assert(amplitude % 2 == 1)

   self.amplitude = amplitude
   self.quality = quality
   self._gaussian_noise = GaussianNoise(amplitude, quality)
   self._mid_floor = math.floor(amplitude/2)
end

function InverseGaussianNoise:generate()
   local value = self._gaussian_noise:generate()
   if value <= self._mid_floor then
      return self._mid_floor - value -- flip the distribution below the mean
   end
   return self.amplitude + self._mid_floor+1 - value -- flip the distribution above the mean
end


return InverseGaussianNoise
