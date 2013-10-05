local WorldGenerator = require 'services.world_generation.world_generator'
local WorldGenerationService = class()

function WorldGenerationService:__init()
end

function WorldGenerationService:create_world(use_async)
   --self:test_filter()

   local wg = WorldGenerator()
   wg:create_world(use_async)
   return wg
end

-----

local Array2D = require 'services.world_generation.array_2D'
local HeightMapRenderer = require 'services.world_generation.height_map_renderer'
local GaussianRandom = require 'services.world_generation.math.gaussian_random'
local MathFns = require 'services.world_generation.math.math_fns'
local FilterFns = require 'services.world_generation.filter.filter_fns'
local BoundaryNormalizingFilter = require 'services.world_generation.filter.boundary_normalizing_filter'

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

function WorldGenerationService:test_filter()
   local size = 64
   local noise_map = Array2D(size, size)
   local filtered_map = Array2D(size, size)
   local height_map_renderer = HeightMapRenderer(size)

   noise_map:process_map(function () return GaussianRandom.generate(24, 32) end)
   --noise_map:clear(0); noise_map:set(32, 32, 200)

   FilterFns.filter_2D_generic(filtered_map, noise_map, size, size, filter_kernel_8_025)

   filtered_map:process_map(function (value) return value/8 end)

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

