-- Generates noise from 0 to amplitude following an approximate guassian distribution
-- quality = 1 produces unifurm noise
-- quality = 3 produces a PDF with a continuous first derivative
-- i.e. probability density function is smooth
local GaussianNoise = class()

GaussianNoise.NO_QUALITY = 1
GaussianNoise.LOW_QUALITY = 3
GaussianNoise.MED_QUALITY = 6
-- this implementation too expensive for high quality gaussian noise
-- do something like Box-Muller transform if you want high quality Gaussion noise

function GaussianNoise:__init(amplitude, quality)
   self.amplitude = amplitude
   self.quality = quality
   self._component_amplitude = amplitude/quality
end

function GaussianNoise:generate()
   local i
   local sum = 0

   for i=1, self.quality, 1 do
      --sum = sum + math.floor(math.random() * self._component_amplitude + 0.5)
      sum = sum + math.random()
   end
   
   return math.floor(sum * self._component_amplitude + 0.5)
end

return GaussianNoise
