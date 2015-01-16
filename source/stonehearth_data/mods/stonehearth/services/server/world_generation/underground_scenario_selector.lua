local ScenarioSelector = require 'services.server.world_generation.scenario_selector'
local RandomNumberGenerator = _radiant.csg.RandomNumberGenerator
local log = radiant.log.create_logger('underground_scenario_selector')

local UndergroundScenarioSelector = class()

function UndergroundScenarioSelector:__init(feature_size, seed)
   self._feature_size = feature_size
   self._rng = RandomNumberGenerator(seed)
end

function UndergroundScenarioSelector:place_revealed_scenarios(underground_elevation_map, tile_offset_x, tile_offset_y)
end

return UndergroundScenarioSelector
