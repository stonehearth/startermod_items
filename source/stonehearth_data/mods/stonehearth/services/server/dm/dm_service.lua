local Array2D = require 'services.server.world_generation.array_2D'
local Point2 = _radiant.csg.Point2
local log = radiant.log.create_logger('dm_service')

local CombatCurve = require 'services.server.dynamic_scenario.combat_curve'

local DmService = class()

function DmService:initialize()
   self._enable_scenarios = radiant.util.get_config('enable_dynamic_scenarios', true)

   self._starting_location_exclusion_radius = radiant.util.get_config('scenario.starting_location_exclusion_radius', 64)
   self._difficulty_increment_distance = radiant.util.get_config('scenario.difficulty_increment_distance', 256)
   self._combat_curve = CombatCurve(5, 50, 0.95, 1000)

   assert(self._starting_location_exclusion_radius < self._difficulty_increment_distance)

   self._sv = self.__saved_variables:get_data()
   if self._sv._initialized then
      radiant.events.listen_once(radiant, 'radiant:game_loaded', function (e)
         radiant.events.listen(radiant, 'stonehearth:very_slow_poll', self, self._on_think)
      end)
   else 
      radiant.events.listen(radiant, 'radiant:entity:post_create', function (e)
         if e.entity:get_uri() == 'stonehearth:camp_standard' then
            -- Once the camp-standard has been placed, we begin the thinking process!
            self._sv._initialized = true
            self.__saved_variables:mark_changed()
            radiant.events.listen(radiant, 'stonehearth:very_slow_poll', self, self._on_think)
            return radiant.events.UNLISTEN
         end
      end)
   end
end

function DmService:_on_think()
   -- TODO: a complex algorithm.

   if not self._enable_scenarios then
      return
   end

   self._combat_curve:update(stonehearth.calendar:get_elapsed_time())

   if self._combat_curve:willing_to_spawn() then
      stonehearth.dynamic_scenario:try_spawn_scenario(self._combat_curve:get_scenario_types())
   end

   --[[local scenario_kind = 'combat'

   -- Some hackiness until proper military strength comes in.
   local civ_military = stonehearth.score:get_scores_for_player('player_1'):get_score_data()['happiness']['military']
   local scenario_min_difficulty = civ_military - 5
   local scenario_max_difficulty = civ_military + 5

   if stonehearth.dynamic_scenario:num_running_scenarios() < 1 then
      stonehearth.dynamic_scenario:spawn_scenario(scenario_kind, scenario_min_difficulty, scenario_max_difficulty)
   end]]
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
