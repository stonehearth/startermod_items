local FilterFns = class()

local Array2DFns = require 'filter.array_2D_fns'
local BoundaryNormalizingFilter = require 'filter.boundary_normalizing_filter'

local x = {}
local y = {}
local temp = {}

function FilterFns.filter_2D_generic(dst, src, src_width, src_height, filter_kernel, sampling_interval)
   if sampling_interval == nil then sampling_interval = 1 end

   local i, j
   local dst_width, dst_height

   -- horizontal
   dst_width = FilterFns.calc_resampled_length(src_width, sampling_interval)
   for i=1, src_height, 1 do
      Array2DFns.get_row_vector(x, src, i, src_width, src_width)
      filter_kernel:filter(y, x, src_width, sampling_interval)
      Array2DFns.set_row_vector(temp, y, i, dst_width, dst_width)
   end

   -- after horizontal downsampling, temp now has dimensions of dst_width x src_height

   -- vertical
   dst_height = FilterFns.calc_resampled_length(src_height, sampling_interval)
   for j=1, dst_width, 1 do
      Array2DFns.get_column_vector(x, temp, j, dst_width, src_height)
      filter_kernel:filter(y, x, src_height, sampling_interval)
      Array2DFns.set_column_vector(dst, y, j, dst_width, dst_height)
   end
end

-- 4th order, cutoff 0.50
local f_4_050 = {
   -0.0309320191935857,
    0.2917539282879510,
    0.4783561818112700,
    0.2917539282879510,
   -0.0309320191935857
}
local filter_kernel_4_050 = BoundaryNormalizingFilter(f_4_050)

-- iterative downsampling is computationally cheaper than using a long filter
function FilterFns.downsample_2D(dst, src, src_width, src_height, sampling_interval)
   -- only supports powers of 2
   assert(sampling_interval % 2 == 0)

   if sampling_interval == 2 then
      FilterFns.filter_2D_generic(dst, src, src_width, src_height, filter_kernel_4_050, 2)
      return
   end

   local temp = {}
   FilterFns.filter_2D_generic(temp, src, src_width, src_height, filter_kernel_4_050, 2)
   FilterFns.downsample_2D(dst, temp, src_width*0.5, src_height*0.5, sampling_interval*0.5)
end

-- 4th order, cutoff 0.25
local f_4_025 = {
   0.15277073060280900,
   0.22262993752905500,
   0.24919866373627200,
   0.22262993752905500,
   0.15277073060280900
}
local filter_kernel_4_025 = BoundaryNormalizingFilter(f_4_025)

function FilterFns.filter_2D_025(dst, src, src_width, src_height)
   FilterFns.filter_2D_generic(dst, src, src_width, src_height, filter_kernel_4_025, 1)
end

function FilterFns.calc_resampled_length(src_len, sampling_interval)
   return math.ceil(src_len / sampling_interval)
end

----------

function FilterFns._test()
   FilterFns._test_basic()
   FilterFns._test_iterative_downsample()
end

function FilterFns._test_basic()
   local delta = 1e-14
   local sampling_interval = 2
   local src_width = 16
   local src_height = 8
   local src = {}
   local filter_kernel = filter_kernel_4_025
   local dst_width = FilterFns.calc_resampled_length(src_width, sampling_interval)
   local dst_height = FilterFns.calc_resampled_length(src_height, sampling_interval)
   local dst = {}
   local i

   for i=1, src_width*src_height, 1 do
      src[i] = 1
   end

   FilterFns.filter_2D_generic(dst, src, src_width, src_height, filter_kernel, sampling_interval)

   local i
   local value = filter_kernel._f_sum^2
   for i=1, dst_width*dst_height, 1 do
      assert(math.abs(value-dst[i]) < delta)
   end
end

function FilterFns._test_iterative_downsample()
   local delta = 1e-14
   local sampling_interval = 4
   local src_width = 16
   local src_height = 16
   local src = {}
   local dst_width = src_width / sampling_interval
   local dst_height = src_height / sampling_interval
   local dst = {}
   local i

   for i=1, src_width*src_height, 1 do
      src[i] = 1
   end

   FilterFns.downsample_2D(dst, src, src_width, src_height, sampling_interval)

   local i
   local filter_kernel = filter_kernel_4_050 -- needs to match the filter in downsample_2D
   local value = filter_kernel._f_sum^2
   for i=1, dst_width*dst_height, 1 do
      assert(math.abs(value-dst[i]) < delta)
   end
end

return FilterFns

