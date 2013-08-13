local Wavelet = class()
local CDF_97 = radiant.mods.require('/stonehearth_terrain/wavelet/cdf_97.lua')
--local Daubechies_D4 = radiant.mods.require('/stonehearth_terrain/wavelet/daubechies_d4.lua')
--local Haar = radiant.mods.require('/stonehearth_terrain/wavelet/haar.lua')

local transform = CDF_97

function Wavelet:__init()
end

function Wavelet:DWT_2D(src, src_width, src_height, current_level, max_level)
   if current_level > max_level then return end
   local i, j
   local vec = {}
   local level_width, level_height =
      self:get_dimensions_for_level(src_width, src_height, current_level)

   -- horizontal transform
   for i=1, level_height, 1 do
      self:_get_row_vector(vec, src, i, src_width, level_width)
      transform:DWT_1D(vec, level_width)
      self:_set_row_vector(src, vec, i, src_width, level_width)
   end

   -- vertical transform
   for j=1, level_width, 1 do
      self:_get_column_vector(vec, src, j, src_width, level_height)
      transform:DWT_1D(vec, level_height)
      self:_set_column_vector(src, vec, j, src_width, level_height)
   end

   self:DWT_2D(src, src_width, src_height, current_level+1, max_level)
end

-- if min_level not specified, will perform full inverse
function Wavelet:IDWT_2D(src, src_width, src_height, current_level, min_level)
   if not min_level then min_level = 1 end
   if current_level < min_level then return end
   local i, j
   local vec = {}
   local level_width, level_height =
      self:get_dimensions_for_level(src_width, src_height, current_level)

   -- vertical transform
   for j=1, level_width, 1 do
      self:_get_column_vector(vec, src, j, src_width, level_height)
      transform:IDWT_1D(vec, level_height)
      self:_set_column_vector(src, vec, j, src_width, level_height)
   end

   -- horizontal transform
   for i=1, level_height, 1 do
      self:_get_row_vector(vec, src, i, src_width, level_width)
      transform:IDWT_1D(vec, level_width)
      self:_set_row_vector(src, vec, i, src_width, level_width)
   end

   self:IDWT_2D(src, src_width, src_height, current_level-1, min_level)
end

function Wavelet:get_dimensions_for_level(width, height, level)
   local scaling_factor = 0.5^(level-1)
   local scaled_width = width * scaling_factor
   local scaled_height = height * scaling_factor
   return scaled_width, scaled_height
end

function Wavelet:_copy_vector(dst, src, dst_start, dst_inc, src_start, src_inc, length)
   local i
   local x = src_start
   local y = dst_start

   for i=1, length, 1 do
      dst[y] = src[x]
      x = x + src_inc
      y = y + dst_inc
   end
end

function Wavelet:_get_row_vector(dst, src, row_num, row_width, length)
   self:_copy_vector(dst, src, 1, 1, (row_num-1)*row_width+1, 1, length)
end

function Wavelet:_set_row_vector(dst, src, row_num, row_width, length)
   self:_copy_vector(dst, src, (row_num-1)*row_width+1, 1, 1, 1, length)
end

function Wavelet:_get_column_vector(dst, src, col_num, row_width, length)
   self:_copy_vector(dst, src, 1, 1, col_num, row_width, length)
end

function Wavelet:_set_column_vector(dst, src, col_num, row_width, length)
   self:_copy_vector(dst, src, col_num, row_width, 1, 1, length)
end

---------

function Wavelet:_test_perfect_reconstruction()
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

   self:DWT_2D(y, width, height, 1, levels)
   self:IDWT_2D(y, width, height, levels, 1)

   local diff
   for i=1, width*height, 1 do
      diff = math.abs(y[i]-x[i])
      assert(diff < DELTA)
   end
end

return Wavelet

