local Array2D = require 'services.world_generation.array_2D'
local MathFns = require 'services.world_generation.math.math_fns'
local HabitatType = require 'services.world_generation.habitat_type'
local ActivationType = require 'services.scenario.activation_type'
local ScenarioSelector = require 'services.scenario.scenario_selector'
local ScenarioModderServices = require 'services.scenario.scenario_modder_services'
local Timer = require 'services.world_generation.timer'
local Point2 = _radiant.csg.Point2
local Rect2 = _radiant.csg.Rect2
local Region2 = _radiant.csg.Region2
local _terrain = radiant._root_entity:add_component('terrain')
local log = radiant.log.create_logger('scenario_service')

local ScenarioService = class()

function ScenarioService:__init()
end

function ScenarioService:initialize()
   log:write(0, 'initialize not implemented for scenario service!')
end

function ScenarioService:create_new_game(feature_size, rng)
   self._feature_size = feature_size
   self._rng = rng
   self._difficulty_increment_distance = radiant.util.get_config('scenario.difficulty_increment_distance', 256)
   self._starting_location_exclusion_radius = radiant.util.get_config('scenario.starting_location_exclusion_radius', 64)
   -- use reduced spawn range until fog of war is implemented
   --self._reveal_distance = radiant.util.get_config('sight_radius', 64) * 2
   self._reveal_distance = radiant.util.get_config('sight_radius', 64) + 8
   self._revealed_region = Region2()
   self._dormant_scenarios = {}
   self._last_optimized_rect_count = 10
   self._region_optimization_threshold = radiant.util.get_config('region_optimization_threshold', 1.2)

   assert(self._starting_location_exclusion_radius < self._difficulty_increment_distance)

   local scenario_index = radiant.resources.load_json('stonehearth:scenarios:scenario_index')
   local categories = {}
   local properties, category, error_message

   -- load all the categories
   for name, properties in pairs(scenario_index.categories) do
      -- parse activation type
      if not ActivationType.is_valid(properties.activation_type) then
         log:error('Error parsing "%s": Invalid activation_type "%s".', file, tostring(properties.activation_type))
      end

      categories[name] = ScenarioSelector(properties.frequency, properties.priority, properties.activation_type, self._rng)
   end

   -- load the scenarios into the categories
   for _, file in pairs(scenario_index.scenarios) do
      properties = radiant.resources.load_json(file)

      -- parse category
      category = categories[properties.category]
      if category then
         category:add(properties)
      else
         log:error('Error parsing "%s": Category "%s" has not been defined.', file, tostring(properties.category))
      end

      -- parse habitat types
      error_message = self:_parse_habitat_types(properties)
      if error_message then
         log:error('Error parsing "%s": "%s"', file, error_message)
      end

      -- parse difficulty
      error_message = self:_parse_difficulty(properties)
      if error_message then
         log:error('Error parsing "%s": "%s"', file, error_message)
      end
   end

   self._categories = categories

   self:_register_events()
end

function ScenarioService:_register_events()
   -- TODO: in multiplayer, scenario service needs to reveal for all factions.  we actually need
   -- to iterate over all player popluations rather than just the hardcoded "player_1".  When the
   -- service to iterate over all players in the game is written, change this bit. -- tony
   self._faction = stonehearth.population:get_population('player_1')
   radiant.events.listen(radiant, 'stonehearth:very_slow_poll', self, self._on_poll)
end

function ScenarioService:_on_poll()
   self:_reveal_around_entities()
end

function ScenarioService:place_static_scenarios(habitat_map, elevation_map, tile_offset_x, tile_offset_y)
   local scenarios = self:_select_scenarios(habitat_map, 'static')

   self:_place_scenarios(scenarios, habitat_map, elevation_map, nil, tile_offset_x, tile_offset_y,
      function (scenario_info)
         local si = scenario_info
         self:_activate_scenario(si.properties, si.offset_x, si.offset_y)
      end
   )
end

function ScenarioService:place_revealed_scenarios(habitat_map, elevation_map, tile_offset_x, tile_offset_y)
   local difficulty_map = self:_derive_difficulty_map(habitat_map, tile_offset_x, tile_offset_y)
   local scenarios = self:_select_scenarios(habitat_map, 'revealed')

   self:_place_scenarios(scenarios, habitat_map, elevation_map, difficulty_map, tile_offset_x, tile_offset_y,
      function (scenario_info)
         self:_mark_scenario_map(self._dormant_scenarios, scenario_info, scenario_info)
      end
   )
end

function ScenarioService:set_starting_location(location)
   self._starting_location = location
   local x = self._starting_location.x
   local y = self._starting_location.y

   -- hack to remove wildlife around banner
   -- this should go away when we place scenarios after the banner is down
   local reveal_distance = self._reveal_distance
   local region = Region2()

   region:add_cube(
      Rect2(
         -- remember +1 on max
         Point2(x-reveal_distance,   y-reveal_distance),
         Point2(x+reveal_distance+1, y+reveal_distance+1)
      )
   )

   -- self:reveal_region(region,
   --    function (scenario_properties)
   --       --return true -- CHECKCHECK
   --       return scenario_properties.category ~= 'wildlife'
   --    end
   -- )
end

function ScenarioService:reveal_region(world_space_region, activation_filter)
   local revealed_region = self._revealed_region
   local bounded_world_space_region, unrevealed_region, new_region
   local key, scenario_info, properties

   bounded_world_space_region = self:_bound_region_by_terrain(world_space_region)
   new_region = self:_region_to_habitat_space(bounded_world_space_region)

   unrevealed_region = new_region - revealed_region

   for rect in unrevealed_region:each_cube() do
      for j = rect.min.y, rect.max.y-1 do
         for i = rect.min.x, rect.max.x-1 do
            key = self:_get_coordinate_key(i, j)

            scenario_info = self._dormant_scenarios[key]
            if scenario_info ~= nil then
               properties = scenario_info.properties

               self:_mark_scenario_map(self._dormant_scenarios, nil, scenario_info)

               -- hack until we make place non-static scenarios after banner placement
               local activate = true
               if activation_filter then
                  activate = activation_filter(properties)
               end
               if activate then
                  local seconds = Timer.measure(
                     function()
                        self:_activate_scenario(properties, scenario_info.offset_x, scenario_info.offset_y)
                     end
                  )
                  log:info('Activated scenario "%s" in %.3fs', properties.name, seconds)
               end
            end
         end
      end
   end

   revealed_region:add_unique_region(unrevealed_region)

   local num_rects = revealed_region:get_num_rects()

   if num_rects >= self._last_optimized_rect_count * self._region_optimization_threshold then
      log:debug('Optimizing scenario region')
      revealed_region:optimize_by_oct_tree(8)
      self._last_optimized_rect_count = revealed_region:get_num_rects()
   end
end

function ScenarioService:_reveal_around_entities()
   local reveal_distance = self._reveal_distance
   local citizens = self._faction:get_citizens()
   local region = Region2()
   local mob, pt, rect

   for _, entity in pairs(citizens) do
      mob = entity:get_component('mob')

      if mob then
         pt = mob:get_world_grid_location()

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

function ScenarioService:_bound_region_by_terrain(region)
   local terrain_bounds = self:_get_terrain_region()
   return _radiant.csg.intersect_region2(region, terrain_bounds)
end

 -- this will eventually be a non-rectangular region composed of the tiles that have been generated
function ScenarioService:_get_terrain_region()
   local region = Region2()
   local bounds = _terrain:get_bounds():project_onto_xz_plane()
   region:add_cube(bounds)
   return region
end

function ScenarioService:_mark_scenario_map(map, value, scenario_info)
   local properties = scenario_info.properties
   local feature_x, feature_y, feature_width, feature_length, key

   feature_x, feature_y = self:_get_feature_space_coords(scenario_info.offset_x, scenario_info.offset_y)
   feature_width, feature_length = self:_get_dimensions_in_feature_units(properties.size.width, properties.size.length)

   -- i, j are offsets so using base 0
   for j=0, feature_length-1 do
      for i=0, feature_width-1 do
         -- dormant scenarios could also be implemented with nested tables
         key = self:_get_coordinate_key(feature_x + i, feature_y + j)
         map[key] = value
      end
   end
end

-- this may eventually move outside of this class
function ScenarioService:_derive_difficulty_map(habitat_map, tile_offset_x, tile_offset_y)
   local feature_size = self._feature_size
   local starting_location = self._starting_location
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

function ScenarioService:_place_scenarios(scenarios, habitat_map, elevation_map, difficulty_map, tile_offset_x, tile_offset_y, place_fn)
   local rng = self._rng
   local feature_size = self._feature_size
   local feature_width, feature_length, voxel_width, voxel_length
   local site, sites, num_sites, roll, habitat_types, difficulty_range, residual_x, residual_y
   local offset_x, offset_y, feature_offset_x, feature_offset_y, intra_cell_offset_x, intra_cell_offset_y
   local scenario_info

   for _, properties in pairs(scenarios) do
      habitat_types = properties.habitat_types
      difficulty_range = properties.difficulty

      -- dimensions of the scenario in voxels
      voxel_width = properties.size.width
      voxel_length = properties.size.length

      -- get dimensions of the scenario in feature cells
      feature_width, feature_length = self:_get_dimensions_in_feature_units(voxel_width, voxel_length)

      -- get a list of valid locations
      sites, num_sites = self:_find_valid_sites(habitat_map, elevation_map, difficulty_map, habitat_types, difficulty_range, feature_width, feature_length)

      if num_sites > 0 then
         -- pick a random location
         roll = rng:get_int(1, num_sites)
         site = sites[roll]

         feature_offset_x = (site.i-1)*feature_size
         feature_offset_y = (site.j-1)*feature_size

         residual_x = feature_width*feature_size - voxel_width
         residual_y = feature_length*feature_size - voxel_length

         intra_cell_offset_x = rng:get_int(0, residual_x)
         intra_cell_offset_y = rng:get_int(0, residual_y)

         -- these are in C++ base 0 array coordinates
         offset_x = tile_offset_x + feature_offset_x + intra_cell_offset_x
         offset_y = tile_offset_y + feature_offset_y + intra_cell_offset_y

         scenario_info = {
            properties = properties,
            offset_x = offset_x,
            offset_y = offset_y
         }

         place_fn(scenario_info)

         self:_mark_habitat_map(habitat_map, site.i, site.j, feature_width, feature_length)

         if properties.unique then
            self:_remove_scenario_from_selector(properties)
         end
      end
   end
end

-- TODO: randomize orientation in place_entity
function ScenarioService:_activate_scenario(properties, offset_x, offset_y)
   local services, scenario_script

   services = ScenarioModderServices(self._rng)
   services:_set_scenario_properties(properties, offset_x, offset_y)

   scenario_script = radiant.mods.load_script(properties.script)
   scenario_script()
   scenario_script:initialize(properties, services)
end

function ScenarioService:_find_valid_sites(habitat_map, elevation_map, difficulty_map, habitat_types, difficulty_range, width, length)
   local i, j, is_habitat_type, is_flat, is_correct_difficulty, elevation
   local sites = {}
   local num_sites = 0
   local is_correct_difficulty = true
   local min_difficulty = nil
   local max_difficulty = nil

   if difficulty_range then
      min_difficulty = difficulty_range.min
      max_difficulty = difficulty_range.max
   end

   local is_target_difficulty = function(value)
      if value == 'x' then
         return false
      end

      if min_difficulty and value < min_difficulty then
         return false
      end

      if max_difficulty and value > max_difficulty then
         return false
      end

      return true
   end

   local is_target_habitat_type = function(value)
      local found = habitat_types[value]
      return found
   end

   for j=1, habitat_map.height-(length-1) do
      for i=1, habitat_map.width-(width-1) do
         -- check if block meets habitat requirements
         is_habitat_type = habitat_map:visit_block(i, j, width, length, is_target_habitat_type)

         if is_habitat_type then
            -- check if block is flat
            elevation = elevation_map:get(i, j)

            is_flat = elevation_map:visit_block(i, j, width, length,
               function (value)
                  return value == elevation
               end
            )

            if is_flat then
               if difficulty_map then
                  is_correct_difficulty = difficulty_map:visit_block(i, j, width, length, is_target_difficulty)
               end

               if is_correct_difficulty then
                  num_sites = num_sites + 1
                  sites[num_sites] = { i = i, j = j}
               end
            end
         end
      end
   end

   return sites, num_sites
end

-- get a list of scenarios from all the categories
function ScenarioService:_select_scenarios(habitat_map, activation_type)
   local categories = self._categories
   local selected_scenarios = {}
   local selector, list

   for key, _ in pairs(categories) do
      selector = categories[key]

      if selector.activation_type == activation_type then
         list = selector:select_scenarios(habitat_map)

         for _, properties in pairs(list) do
            table.insert(selected_scenarios, properties)
         end
      end
   end

   self:_sort_scenarios(selected_scenarios)

   return selected_scenarios
end

-- order first by priority, then by area, then by weight
function ScenarioService:_sort_scenarios(scenarios)
   local comparator = function(a, b)
      local category_a = a.category
      local category_b = b.category

      if category_a ~= category_b then
         local categories = self._categories
         local priority_a = categories[category_a].priority
         local priority_b = categories[category_b].priority
         -- higher priority sorted to lower index
         return priority_a > priority_b
      end

      local area_a = a.size.width * a.size.length
      local area_b = b.size.width * b.size.length
      if area_a ~= area_b then
         -- larger area sorted to lower index
         return area_a > area_b 
      end

      return a.weight > b.weight
   end

   table.sort(scenarios, comparator)
end

function ScenarioService:_remove_scenario_from_selector(properties)
   local name = properties.name
   local category = properties.category

   -- just remove from future selection, don't remove from master index
   self._categories[category]:remove(name)
end

function ScenarioService:_mark_habitat_map(habitat_map, i, j, width, length)
   habitat_map:set_block(i, j, width, length, HabitatType.occupied)
end

function ScenarioService:_region_to_habitat_space(region)
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

function ScenarioService:_rect_to_habitat_space(rect)
   local feature_size = self._feature_size
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

function ScenarioService:_get_dimensions_in_feature_units(width, length)
   local feature_size = self._feature_size
   return math.ceil(width/feature_size), math.ceil(length/feature_size)
end

function ScenarioService:_get_feature_space_coords(x, y)
   local feature_size = self._feature_size
   return math.floor(x/feature_size),
          math.floor(y/feature_size)
end

function ScenarioService:_get_coordinate_key(x, y)
   return string.format('%d,%d', x, y)
end

-- parse the habitat_types array into a set so we can index by the habitat_type
function ScenarioService:_parse_habitat_types(properties)
   local strings = properties.habitat_types
   local habitat_type
   local habitat_types = {}
   local error_message = nil

   for _, value in pairs(strings) do
      if HabitatType.is_valid(value) then
         habitat_types[value] = value
      else
         -- concatenate multiple errors into a single string
         if error_message == nil then
            error_message = ''
         end
         error_message = string.format('%s Invalid habitat type "%s".', error_message, tostring(value))
      end
   end

   properties.habitat_types = habitat_types

   return error_message
end

function ScenarioService:_parse_difficulty(properties)
   local max_difficulty = 99
   local difficulty = properties.difficulty

   if not difficulty then
      difficulty = {}
      properties.difficulty = difficulty
   end

   if not difficulty.min then
      difficulty.min = 0
   end

   if not difficulty.max then
      difficulty.max = max_difficulty
   end

   return nil
end

return ScenarioService
