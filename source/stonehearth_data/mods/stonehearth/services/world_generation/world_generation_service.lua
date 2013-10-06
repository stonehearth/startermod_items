local WorldGenerator = require 'services.world_generation.world_generator'
local WorldGenerationService = class()

function WorldGenerationService:__init()
end

function WorldGenerationService:create_world(async)
   --self:test_filter()

   local wg = WorldGenerator(async)
   wg:create_world()
   return wg
end

-----

-- TEST CODE

local Array2D = require 'services.world_generation.array_2D'
local HeightMapRenderer = require 'services.world_generation.height_map_renderer'
local GaussianRandom = require 'services.world_generation.math.gaussian_random'
local MathFns = require 'services.world_generation.math.math_fns'
local FilterFns = require 'services.world_generation.filter.filter_fns'
local BoundaryNormalizingFilter = require 'services.world_generation.filter.boundary_normalizing_filter'
local Wavelet = require 'services.world_generation.wavelet.wavelet'
local WaveletFns = require 'services.world_generation.wavelet.wavelet_fns'

local filter_kernel_8_025 = BoundaryNormalizingFilter({
9.27968054988339e-005,
1.93115946757266e-002,
1.02082532761821e-001,
2.30618166993187e-001,
2.95789817527533e-001,
2.30618166993187e-001,
1.02082532761821e-001,
1.93115946757266e-002,
9.27968054988339e-005
})

local filter_kernel_10_025 = BoundaryNormalizingFilter({
-3.81677788641566e-003,
1.76448116941646e-004,
3.24259249652166e-002,
1.16864651139762e-001,
2.20320308451122e-001,
2.68058890426748e-001,
2.20320308451122e-001,
1.16864651139762e-001,
3.24259249652166e-002,
1.76448116941646e-004,
-3.81677788641566e-003
})

local filter_kernel_16_025 = BoundaryNormalizingFilter({
-7.77956066401648e-005,
-3.76115182543916e-003,
-1.13425151147566e-002,
-1.60635790055983e-002,
5.25120344821112e-004,
5.39858013878338e-002,
1.37129510913712e-001,
2.15617860823305e-001,
2.47973496165525e-001,
2.15617860823305e-001,
1.37129510913712e-001,
5.39858013878338e-002,
5.25120344821112e-004,
-1.60635790055983e-002,
-1.13425151147566e-002,
-3.76115182543917e-003,
-7.77956066401648e-005
})

local filter_kernel_32_0125 = BoundaryNormalizingFilter({
-7.78190247451533e-005,
-7.97947281792747e-004,
-1.91984788934255e-003,
-3.60623462594897e-003,
-5.67104202895128e-003,
-7.46577210362419e-003,
-7.90711381099668e-003,
-5.66403998060921e-003,
5.25278417029785e-004,
1.14799220646602e-002,
2.72450143418953e-002,
4.69010243286112e-002,
6.85828126082718e-002,
8.97271535404976e-002,
1.07508492546964e-001,
1.19371268006615e-001,
1.23537701782931e-001,
1.19371268006615e-001,
1.07508492546964e-001,
8.97271535404976e-002,
6.85828126082718e-002,
4.69010243286112e-002,
2.72450143418953e-002,
1.14799220646602e-002,
5.25278417029785e-004,
-5.66403998060920e-003,
-7.90711381099668e-003,
-7.46577210362419e-003,
-5.67104202895128e-003,
-3.60623462594897e-003,
-1.91984788934255e-003,
-7.97947281792747e-004,
-7.78190247451533e-005
})

function WorldGenerationService:test_filter()
   math.randomseed(1)

   local size = 64
   local noise_map = Array2D(size, size)
   local filtered_map = Array2D(size, size)
   local height_map_renderer = HeightMapRenderer(size)

   noise_map:process_map(function () return GaussianRandom.generate(96, 128) end)
   --noise_map:clear(0); noise_map:set(32, 32, 200)

   -- filtered_map = noise_map
   -- WaveletFns.scale_high_freq(filtered_map, size, size, 0.05, 3)
   -- Wavelet.IDWT_2D(filtered_map, size, size, 3)
   -- filtered_map:process_map(function (value) return value/4 end)

   FilterFns.filter_2D_generic(filtered_map, noise_map, size, size, filter_kernel_32_0125)

   filtered_map:process_map(function (value) return value/32 end)

   filtered_map:process_map(function (value) return MathFns.round(value) end)
   height_map_renderer:visualize_height_map(filtered_map)
end

function WorldGenerationService:tesselator_test()
   HeightMapRenderer.tesselator_test()   
end

function WorldGenerationService:run_unit_tests()
   BoundaryNormalizingFilter._test()
   FilterFns._test()
   CDF_97._test()
   Wavelet._test()
end

return WorldGenerationService()

