local Array2D = require 'services.server.world_generation.array_2D'
local Point2 = _radiant.csg.Point2
local log = radiant.log.create_logger('dm_service')

local DmService = class()

function DmService:initialize()
   self._sv = self.__saved_variables:get_data()

   self._enable_scenarios = radiant.util.get_config('enable_dynamic_scenarios', true)

   if self._sv._initialized then
      radiant.events.listen_once(radiant, 'radiant:game_loaded', function (e)
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
   self._sv._pace_keepers = {}

   local pop_pacekeeper = radiant.create_controller('stonehearth:population_pace_keeper')
   self._sv._pace_keepers['population'] = radiant.create_controller('stonehearth:pace_keeper', pop_pacekeeper)

   self.__saved_variables:mark_changed()
end

function DmService:_on_think()
   if not self._enable_scenarios then
      return
   end

   local current_time = stonehearth.calendar:get_elapsed_time()

   for _, keeper in pairs(self._sv._pace_keepers) do
      keeper:update(current_time)
   end

   local best_keeper = nil
   local keeper_kind = nil
   for pace_kind, keeper in pairs(self._sv._pace_keepers) do
      if keeper:willing_to_spawn() and (not best_keeper or best_keeper:get_buildup_value() < keeper:get_buildup_value()) then
         best_keeper = keeper
         keeper_kind = pace_kind
      end
   end

   -- This way, we only possibly spawn one scenario per think, and it'll always be from the 
   -- least-used pace-keeper.
   if best_keeper then
      if not stonehearth.dynamic_scenario:try_spawn_scenario(keeper_kind, self._sv._pace_keepers) then
         -- We tried to spawn a scenario, but didn't.  There could be a number of reasons why:
         -- nothing compatibile with the difficulty, nothing ready to run, or just luck due to rarity.
         -- We penalize this keeper to ensure that it doesn't throttle other keepers from spawning
         -- scenarios.
         best_keeper:penalize_buildup()
      end
   end
end

return DmService
