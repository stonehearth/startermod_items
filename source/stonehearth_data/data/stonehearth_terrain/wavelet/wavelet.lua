local Wavelet = {}
local Array2DFns = radiant.mods.require('/stonehearth_terrain/filter/array_2D_fns.lua')
local CDF_97 = radiant.mods.require('/stonehearth_terrain/wavelet/cdf_97.lua')

local transform = CDF_97

function Wavelet.DWT_2D(src, src_width, src_height, current_level, max_level)
   if current_level > max_level then return end
   local i, j
   local vec = {}
   local level_width, level_height =
      Wavelet.get_dimensions_for_level(src_width, src_height, current_level)

   -- horizontal transform
   for i=1, level_height, 1 do
      Array2DFns.get_row_vector(vec, src, i, src_width, level_width)
      transform.DWT_1D(vec, level_width)
      Array2DFns.set_row_vector(src, vec, i, src_width, level_width)
   end

   -- vertical transform
   for j=1, level_width, 1 do
      Array2DFns.get_column_vector(vec, src, j, src_width, level_height)
      transform.DWT_1D(vec, level_height)
      Array2DFns.set_column_vector(src, vec, j, src_width, level_height)
   end

   Wavelet.DWT_2D(src, src_width, src_height, current_level+1, max_level)
end

-- if min_level not specified, will perform full inverse
function Wavelet.IDWT_2D(src, src_width, src_height, current_level, min_level)
   if not min_level then min_level = 1 end
   if current_level < min_level then return end
   local i, j
   local vec = {}
   local level_width, level_height =
      Wavelet.get_dimensions_for_level(src_width, src_height, current_level)

   -- vertical transform
   for j=1, level_width, 1 do
      Array2DFns.get_column_vector(vec, src, j, src_width, level_height)
      transform.IDWT_1D(vec, level_height)
      Array2DFns.set_column_vector(src, vec, j, src_width, level_height)
   end

   -- horizontal transform
   for i=1, level_height, 1 do
      Array2DFns.get_row_vector(vec, src, i, src_width, level_width)
      transform.IDWT_1D(vec, level_width)
      Array2DFns.set_row_vector(src, vec, i, src_width, level_width)
   end

   Wavelet.IDWT_2D(src, src_width, src_height, current_level-1, min_level)
end

function Wavelet.get_dimensions_for_level(width, height, level)
   local scaling_factor = 0.5^(level-1)
   local scaled_width = width * scaling_factor
   local scaled_height = height * scaling_factor
   return scaled_width, scaled_height
end

---------

function Wavelet._test_perfect_reconstruction()
   local width, height, levels
   local x = {}
   local y = {}
   local i
   local DELTA = 1e-11

   width = 32
   height = 64
   levels = 3

   for i=1, width*height, 1 do
      x[i] = math.random(0, 100)
   end

   for i=1, width*height, 1 do
      y[i] = x[i]
   end

   Wavelet.DWT_2D(y, width, height, 1, levels)
   Wavelet.IDWT_2D(y, width, height, levels, 1)

   local diff
   for i=1, width*height, 1 do
      diff = math.abs(y[i]-x[i])
      assert(diff < DELTA)
   end
end

return Wavelet

