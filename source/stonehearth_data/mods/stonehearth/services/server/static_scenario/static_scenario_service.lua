local ScenarioModderServices = require 'services.server.static_scenario.scenario_modder_services'
local Timer = require 'services.server.world_generation.timer'
local Point2 = _radiant.csg.Point2
local Rect2 = _radiant.csg.Rect2
local Region2 = _radiant.csg.Region2
local RandomNumberGenerator = _radiant.csg.RandomNumberGenerator
local log = radiant.log.create_logger('static_scenario_service')

local StaticScenarioService = class()

function StaticScenarioService:initialize()
   self._reveal_distance = radiant.util.get_config('sight_radius', 64) + 8
   self._region_optimization_threshold = radiant.util.get_config('region_optimization_threshold', 1.2)
   self._terrain_component = radiant._root_entity:add_component('terrain')

   self._sv = self.__saved_variables:get_data()

   if not self._sv.initialized then
   else
      radiant.events.listen(radiant, 'radiant:game_loaded', function()
            self:_register_events()
            return radiant.events.UNLISTEN
         end)
   end
end

function StaticScenarioService:create_new_game(feature_size, seed)
   self._sv.feature_size = feature_size
   self._sv.rng = RandomNumberGenerator(seed)
   self._sv.dormant_scenarios = {}
   self._sv.next_id = 1
   self._sv.revealed_region = Region2()
   self._sv.last_optimized_rect_count = 10
   self._sv._initialized = true
   self.__saved_variables:mark_changed()

   self:_register_events()
end

function StaticScenarioService:get_reveal_distance()
   return self._reveal_distance
end

-- The properties object should have a 'script' field that points to a lua file.
-- The class returned by the script should support an initialize(properties, services) method.
-- The properties argument is the properties that are passed in here.
-- The services argument will be a ScenarioModderServices object.
function StaticScenarioService:add_scenario(properties, x, y, width, length, activate_now)
   if activate_now then
      self:_activate_scenario(properties, x, y)
   else
      local scenario_info = {
         id = self._sv.next_id,
         x = x,
         y = y,
         width = width,
         length = length,
         properties = properties
      }
      self._sv.next_id = self._sv.next_id + 1
      self.__saved_variables:mark_changed()

      self:_add_to_scenario_map(scenario_info)
   end
end

function StaticScenarioService:_register_events()
   -- TODO: in multiplayer, scenario service needs to reveal for all factions.  we actually need
   -- to iterate over all player popluations rather than just the hardcoded "player_1".  When the
   -- service to iterate over all players in the game is written, change this bit. -- tony
   self._population = stonehearth.population:get_population('player_1')
   radiant.events.listen(radiant, 'stonehearth:very_slow_poll', self, self._on_poll)
end

function StaticScenarioService:_on_poll()
   self:_reveal_around_entities()
end

-- world_space_region is a region2
function StaticScenarioService:reveal_region(world_space_region, activation_filter)
   local revealed_region = self._sv.revealed_region
   local bounded_world_space_region, unrevealed_region, new_region
   local key, scenarios, properties

   bounded_world_space_region = self:_bound_region_by_terrain(world_space_region)
   new_region = self:_region_to_habitat_space(bounded_world_space_region)

   unrevealed_region = new_region - revealed_region

   for rect in unrevealed_region:each_cube() do
      for j = rect.min.y, rect.max.y-1 do
         for i = rect.min.x, rect.max.x-1 do
            key = self:_get_key(i, j)
            scenarios = self._sv.dormant_scenarios[key]

            if scenarios then
               for id, scenario_info in pairs(scenarios) do
                  properties = scenario_info.properties

                  self:_remove_from_scenario_map(scenario_info)

                  local activate = not activation_filter or activation_filter(properties)
                  if activate then
                     local seconds = Timer.measure(function()
                           self:_activate_scenario(properties, scenario_info.x, scenario_info.y)
                        end)
                     log:info('Activated scenario "%s" in %.3fs', properties.name, seconds)
                  end
               end
            end
         end
      end
   end

   revealed_region:add_unique_region(unrevealed_region)

   local num_rects = revealed_region:get_num_rects()

   if num_rects >= self._sv.last_optimized_rect_count * self._region_optimization_threshold then
      log:debug('Optimizing scenario region')
      revealed_region:optimize_by_oct_tree(8)
      self._sv.last_optimized_rect_count = revealed_region:get_num_rects()
   end

   self.__saved_variables:mark_changed()
end

function StaticScenarioService:_reveal_around_entities()
   local reveal_distance = self._reveal_distance
   local citizens = self._population:get_citizens()
   local region = Region2()
   local pt, rect

   for _, entity in pairs(citizens) do
      pt = radiant.entities.get_world_grid_location(entity)

      if pt then
         -- remember +1 on max
         rect = Rect2(
            Point2(pt.x-reveal_distance,   pt.z-reveal_distance),
            Point2(pt.x+reveal_distance+1, pt.z+reveal_distance+1)
         )

         region:add_cube(rect)
      end
   end

   self:reveal_region(region)
end

function StaticScenarioService:_add_to_scenario_map(scenario_info)
   self:_each_key_in(scenario_info.x, scenario_info.y, scenario_info.width, scenario_info.length,
      function(key)
         local scenarios = self._sv.dormant_scenarios[key]

         -- create the list if it doesn't exist
         if not scenarios then
            scenarios = {}
            self._sv.dormant_scenarios[key] = scenarios
         end

         -- add to the list of scenarios at this location
         scenarios[scenario_info.id] = scenario_info
      end)

   self.__saved_variables:mark_changed()
end

function StaticScenarioService:_remove_from_scenario_map(scenario_info)
   self:_each_key_in(scenario_info.x, scenario_info.y, scenario_info.width, scenario_info.length,
      function(key)
         local scenarios = self._sv.dormant_scenarios[key]
         if scenarios then
            -- clear the scenario from the list
            scenarios[scenario_info.id] = nil

            -- remove the list if it is empty
            if not next(scenarios) then
               self._sv.dormant_scenarios[key] = nil
            end
         end
      end)

   self.__saved_variables:mark_changed()
end

function StaticScenarioService:_each_key_in(x, y, width, length, fn)
   local feature_x, feature_y = self:_get_feature_space_coords(x, y)
   local feature_width, feature_length = self:_get_dimensions_in_feature_units(width, length)

   -- i, j are offsets so use base 0
   for j=0, feature_length-1 do
      for i=0, feature_width-1 do
         local key = self:_get_key(feature_x+i, feature_y+j)
         fn(key)
      end
   end
end

-- TODO: randomize orientation in place_entity
function StaticScenarioService:_activate_scenario(properties, x, y)
   local services, scenario_script

   services = ScenarioModderServices(self._sv.rng)
   services:_set_scenario_properties(properties, x, y)

   scenario_script = radiant.mods.load_script(properties.script)
   scenario_script()
   scenario_script:initialize(properties, services)

   self.__saved_variables:mark_changed()
end

function StaticScenarioService:_bound_region_by_terrain(region)
   local terrain_bounds = self:_get_terrain_bounds()
   return region:intersect_region(terrain_bounds)
end

 -- this may eventually be a non-rectangular region composed of the tiles that have been generated
function StaticScenarioService:_get_terrain_bounds()
   local bounds = self._terrain_component:get_bounds():project_onto_xz_plane()
   return Region2(bounds)
end

function StaticScenarioService:_region_to_habitat_space(region)
   local new_region = Region2()
   local num_rects = region:get_num_rects()
   local rect, new_rect

   -- use c++ base 0 array indexing
   for i=0, num_rects-1 do
      rect = region:get_rect(i)
      new_rect = self:_rect_to_habitat_space(rect)
      -- can't use add_unique_cube because of quantization to reduced coordinate space
      new_region:add_cube(new_rect)
   end

   return new_region
end

function StaticScenarioService:_rect_to_habitat_space(rect)
   local feature_size = self._sv.feature_size
   local min, max

   min = Point2(math.floor(rect.min.x/feature_size),
                math.floor(rect.min.y/feature_size))

   if rect:get_area() == 0 then
      max = min
   else
      max = Point2(math.ceil(rect.max.x/feature_size),
                   math.ceil(rect.max.y/feature_size))
   end

   return Rect2(min, max)
end

function StaticScenarioService:_get_dimensions_in_feature_units(width, length)
   local feature_size = self._sv.feature_size
   return math.ceil(width/feature_size), math.ceil(length/feature_size)
end

function StaticScenarioService:_get_feature_space_coords(x, y)
   local feature_size = self._sv.feature_size
   return math.floor(x/feature_size),
          math.floor(y/feature_size)
end

function StaticScenarioService:_get_key(x, y)
   return string.format('%d,%d', x, y)
end

return StaticScenarioService
