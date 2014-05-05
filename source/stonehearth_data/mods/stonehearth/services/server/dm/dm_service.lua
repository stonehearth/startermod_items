local Array2D = require 'services.server.world_generation.array_2D'
local Point2 = _radiant.csg.Point2
local log = radiant.log.create_logger('dm_service')

local DmService = class()

function DmService:initialize()
   self._starting_location_exclusion_radius = radiant.util.get_config('scenario.starting_location_exclusion_radius', 64)
   self._difficulty_increment_distance = radiant.util.get_config('scenario.difficulty_increment_distance', 256)

   assert(self._starting_location_exclusion_radius < self._difficulty_increment_distance)
end

function DmService:derive_difficulty_map(habitat_map, tile_offset_x, tile_offset_y, feature_size, starting_location)
   local difficulty_increment_distance = self._difficulty_increment_distance
   local cell_center = feature_size/2
   local difficulty_map = Array2D(habitat_map.width, habitat_map.height)

   difficulty_map:fill_ij(
      function (i, j)
         local x = tile_offset_x + (i-1)*feature_size + cell_center
         local y = tile_offset_y + (j-1)*feature_size + cell_center
         local tile_center = Point2(x, y)
         local distance = starting_location:distance_to(tile_center)

         if distance < self._starting_location_exclusion_radius then
            -- sentinel that excludes placement
            return 'x'
         end

         local difficulty = math.floor(distance/difficulty_increment_distance)
         return difficulty
      end
   )

   return difficulty_map
end

return DmService
