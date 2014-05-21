local Array2D = require 'services.server.world_generation.array_2D'
local Point2 = _radiant.csg.Point2
local log = radiant.log.create_logger('dm_service')

local CombatCurve = require 'services.server.dynamic_scenario.combat_curve'

local DmService = class()

function DmService:initialize()
   self._sv = self.__saved_variables:get_data()

   self._enable_scenarios = radiant.util.get_config('enable_dynamic_scenarios', true)

   self._starting_location_exclusion_radius = radiant.util.get_config('scenario.starting_location_exclusion_radius', 64)
   self._difficulty_increment_distance = radiant.util.get_config('scenario.difficulty_increment_distance', 256)
   assert(self._starting_location_exclusion_radius < self._difficulty_increment_distance)
   
   if self._sv._initialized then
      radiant.events.listen_once(radiant, 'radiant:game_loaded', function (e)
         self:_init_pace_keepers()
         self.__saved_variables:mark_changed()
         radiant.events.listen(radiant, 'stonehearth:minute_poll', self, self._on_think)
      end)
   else
      self:_init_pace_keepers()
      radiant.events.listen(radiant, 'radiant:entity:post_create', function (e)
         if e.entity:get_uri() == 'stonehearth:camp_standard' then
            -- Once the camp-standard has been placed, we begin the thinking process!
            self._sv._initialized = true
            self.__saved_variables:mark_changed()
            radiant.events.listen(radiant, 'stonehearth:minute_poll', self, self._on_think)
            return radiant.events.UNLISTEN
         end
      end)
   end
end

function DmService:_init_pace_keepers()
   if not self._sv._pace_keepers then
      self._sv._pace_keepers = {}
   end

   local combat_keeper_ds = self._sv._pace_keepers['combat']
   combat_keeper_ds = combat_keeper_ds and combat_keeper_ds or radiant.create_datastore()
   self._sv._pace_keepers['combat'] = CombatCurve(
      function()
         local military_score = stonehearth.score:get_scores_for_player('player_1'):get_score_data().military_strength
         local military_strength = military_score and military_score.total_score or 0

         -- TODO: this is where combat difficulty will get factored in to the code.
         return military_strength * 0.5
      end,
      function()
         local military_score = stonehearth.score:get_scores_for_player('player_1'):get_score_data().military_strength
         local military_strength = military_score and military_score.total_score or 0

         -- TODO: this is where combat difficulty will get factored in to the code.
         return (military_strength + 1) * 1.5
      end,
      function()
         return 0.5
      end,
      function()
         return 1000
      end,
      combat_keeper_ds)
end

function DmService:_on_think()
   if not self._enable_scenarios then
      return
   end

   local current_time = stonehearth.calendar:get_elapsed_time()

   for _, keeper in pairs(self._sv._pace_keepers) do
      keeper:update(current_time)
   end

   for pace_kind, keeper in pairs(self._sv._pace_keepers) do
      -- TODO: keep track of scenarios being spawned this update, so that we don't
      -- spawn more than one of the same kind (hell, might just want to limit the
      -- spawning to one per tick?)
      if keeper:willing_to_spawn() then
         stonehearth.dynamic_scenario:try_spawn_scenario(pace_kind, self._sv._pace_keepers)
      end
   end
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
