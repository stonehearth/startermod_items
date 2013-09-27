local Wavelet = {}
local Array2DFns = require 'filter.array_2D_fns'
local CDF_97 = require 'wavelet.cdf_97'

local transform = CDF_97

-- assumes current_level = 1 if not specified
function Wavelet.DWT_2D(src, src_width, src_height, max_level, current_level)
   if current_level == nil then current_level = 1 end
   if current_level > max_level then return end
   local i, j
   local vec = {}
   local level_width, level_height =
      Wavelet.get_dimensions_for_level(src_width, src_height, current_level)

   -- horizontal transform
   for i=1, level_height do
      Array2DFns.get_row_vector(vec, src, i, src_width, level_width)
      transform.DWT_1D(vec, level_width)
      Array2DFns.set_row_vector(src, vec, i, src_width, level_width)
   end

   -- vertical transform
   for j=1, level_width do
      Array2DFns.get_column_vector(vec, src, j, src_width, level_height)
      transform.DWT_1D(vec, level_height)
      Array2DFns.set_column_vector(src, vec, j, src_width, level_height)
   end

   Wavelet.DWT_2D(src, src_width, src_height, max_level, current_level+1)
end

-- assumes full inverse if min_level not specified
function Wavelet.IDWT_2D(src, src_width, src_height, current_level, min_level)
   if min_level == nil then min_level = 1 end
   if current_level < min_level then return end
   local i, j
   local vec = {}
   local level_width, level_height =
      Wavelet.get_dimensions_for_level(src_width, src_height, current_level)

   -- vertical transform
   for j=1, level_width do
      Array2DFns.get_column_vector(vec, src, j, src_width, level_height)
      transform.IDWT_1D(vec, level_height)
      Array2DFns.set_column_vector(src, vec, j, src_width, level_height)
   end

   -- horizontal transform
   for i=1, level_height do
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

-- test for perfect reconstruction
function Wavelet._test()
   local width, height, levels
   local x = {}
   local y = {}
   local i
   local DELTA = 1e-11

   width = 32
   height = 64
   levels = 3

   for i=1, width*height do
      x[i] = math.random(0, 100)
   end

   for i=1, width*height do
      y[i] = x[i]
   end

   Wavelet.DWT_2D(y, width, height, levels)
   Wavelet.IDWT_2D(y, width, height, levels)

   local diff
   for i=1, width*height do
      diff = math.abs(y[i]-x[i])
      assert(diff < DELTA)
   end
end

return Wavelet

