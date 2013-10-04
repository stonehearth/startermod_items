local FilterFns = class()

local Array2D = require 'services.world_generation.array_2D'
local BoundaryNormalizingFilter = require 'services.world_generation.filter.boundary_normalizing_filter'
local SlewRateFilter = require 'services.world_generation.filter.slew_rate_filter'

function FilterFns.filter_2D_generic(dst, src, src_width, src_height, filter_kernel, sampling_interval)
   if sampling_interval == nil then sampling_interval = 1 end

   local x = {}
   local y = {}
   local temp = {}
   local i, j
   local dst_width, dst_height

   -- horizontal
   dst_width = FilterFns.calc_resampled_length(src_width, sampling_interval)
   for i=1, src_height do
      Array2D:get_row_vector(x, src, i, src_width, src_width)
      filter_kernel:filter(y, x, src_width, sampling_interval)
      Array2D:set_row_vector(temp, y, i, dst_width, dst_width)
   end

   -- after horizontal downsampling, temp now has dimensions of dst_width x src_height

   -- vertical
   dst_height = FilterFns.calc_resampled_length(src_height, sampling_interval)
   for j=1, dst_width do
      Array2D:get_column_vector(x, temp, j, dst_width, src_height)
      filter_kernel:filter(y, x, src_height, sampling_interval)
      Array2D:set_column_vector(dst, y, j, dst_width, dst_height)
   end
end

-- iterative downsampling is computationally cheaper than using a long filter
function FilterFns.downsample_2D(dst, src, src_width, src_height, sampling_interval)
   -- only supports powers of 2
   assert(sampling_interval % 2 == 0)

   if sampling_interval == 2 then
      FilterFns.filter_2D_generic(dst, src, src_width, src_height, filter_kernel_050[6], 2)
      return
   end

   local temp = {}
   FilterFns.filter_2D_generic(temp, src, src_width, src_height, filter_kernel_050[6], 2)
   FilterFns.downsample_2D(dst, temp, src_width*0.5, src_height*0.5, sampling_interval*0.5)
end

-- 4th order, cutoff 0.50
local filter_kernel_050 = {}
filter_kernel_050[4] = BoundaryNormalizingFilter({
   9.26801061194566e-005,
   2.03910230974866e-001,
   5.91994177838029e-001,
   2.03910230974866e-001,
   9.26801061194566e-005
})

-- 6th order, cutoff 0.50
filter_kernel_050[6] = BoundaryNormalizingFilter({
  -8.72491816781095e-003,
   3.11183458641336e-004,
   2.51938335732198e-001,
   5.12950797953944e-001,
   2.51938335732198e-001,
   3.11183458641336e-004,
  -8.72491816781095e-003
})

local filter_kernel_025 = {}

-- 4th order, cutoff 0.25
-- insufficient order to be a good filter
filter_kernel_025[4] = BoundaryNormalizingFilter({
   0.0246354025711508,
   0.2344488306462068,
   0.4818315335652847,
   0.2344488306462068,
   0.0246354025711508
})

-- 6th order, cutoff 0.25
filter_kernel_025[6] = BoundaryNormalizingFilter({
   0.00858724197082233,
   0.06994544733025826,
   0.24494756015038135,
   0.35303950109707594,
   0.24494756015038141,
   0.06994544733025825,
   0.00858724197082233
})

-- 8th order, cutoff 0.25
filter_kernel_025[8] = BoundaryNormalizingFilter({
   0.00009279680549883,
   0.01931159467572660,
   0.10208253276182100,
   0.23061816699318700,
   0.29578981752753300,
   0.23061816699318700,
   0.10208253276182100,
   0.01931159467572660,
   0.00009279680549883
})

function FilterFns.filter_2D_050(dst, src, src_width, src_height, filter_order)
   local filter_kernel = filter_kernel_050[filter_order]
   assert(filter_kernel ~= nil) -- not supported

   FilterFns.filter_2D_generic(dst, src, src_width, src_height, filter_kernel)
end

function FilterFns.filter_2D_025(dst, src, src_width, src_height, filter_order)
   local filter_kernel = filter_kernel_025[filter_order]
   assert(filter_kernel ~= nil) -- not supported
   assert(filter_kernel.filter ~= nil)

   FilterFns.filter_2D_generic(dst, src, src_width, src_height, filter_kernel)
end

local filter_kernel_slew_rate = SlewRateFilter()
function FilterFns.filter_max_slope(dst, src, src_width, src_height)
   FilterFns.filter_2D_generic(dst, src, src_width, src_height, filter_kernel_slew_rate)
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

   for i=1, src_width*src_height do
      src[i] = 1
   end

   FilterFns.filter_2D_generic(dst, src, src_width, src_height, filter_kernel, sampling_interval)

   local i
   local value = filter_kernel._f_sum^2
   for i=1, dst_width*dst_height do
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

   for i=1, src_width*src_height do
      src[i] = 1
   end

   FilterFns.downsample_2D(dst, src, src_width, src_height, sampling_interval)

   local i
   local filter_kernel = filter_kernel_4_050 -- needs to match the filter in downsample_2D
   local value = filter_kernel._f_sum^2
   for i=1, dst_width*dst_height do
      assert(math.abs(value-dst[i]) < delta)
   end
end

return FilterFns

