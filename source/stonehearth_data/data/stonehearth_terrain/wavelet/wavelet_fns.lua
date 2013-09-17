local WaveletFns = {}
local Wavelet = radiant.mods.require('/stonehearth_terrain/wavelet/wavelet.lua')

-- assumes current_level = 1 if not specified
function WaveletFns.scale_high_freq(src, src_width, src_height, power, max_level, current_level)
   if current_level == nil then current_level = 1 end
   if current_level > max_level then return end
   local i, j
   local level_width, level_height =
      Wavelet.get_dimensions_for_level(src_width, src_height, current_level)
   local half_width = level_width/2
   local half_height = level_height/2
   local attenuation = power^(max_level-current_level+1)
   local row_offset = 0

   for j=1, level_height do
      for i=1, level_width do
         if (i > half_width) or (j > half_height) then
            src[row_offset+i] = src[row_offset+i] * attenuation
         end
      end
      row_offset = row_offset + src_width
   end

   WaveletFns.scale_high_freq(src, src_width, src_height, power, max_level, current_level+1)
end

function WaveletFns.calc_attentuation_coefficient(levels, final_coefficient)
   return final_coefficient^(1/levels)
end

function WaveletFns.average_error(src1, src2, width, height)
   local i, j
   local sum = 0
   local rowoffset = 0

   for j=1, height do
      for i=1, width do
         sum = sum + math.abs(src1[rowoffset+i] - src2[rowoffset+i])
      end
      rowoffset = rowoffset + width
   end

   local average = sum / (width*height)
   return average
end
return WaveletFns

