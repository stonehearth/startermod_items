local MathFns = require 'services.world_generation.math.math_fns'
local Point3 = _radiant.csg.Point3
local log = radiant.log.create_logger('world_generation')

local ScenarioServices = class()

-- Modder API starts here
-- methods and properties starting with _ are not intended for modder use
function ScenarioServices:place_entity(uri, x, y)
   x, y = self:_bounds_check(x, y)

   local entity = radiant.entities.create_entity(uri)
   radiant.terrain.place_entity(entity, Point3(x + self._offset_x, 1, y + self._offset_y))
   self:_set_random_facing(entity)
   return entity
end

-- Private API starts here
function ScenarioServices:__init(rng)
   self.rng = rng
end

function ScenarioServices:_set_scenario_properties(properties, offset_x, offset_y)
   self._properties = properties
   self._offset_x = offset_x
   self._offset_y = offset_y
end

function ScenarioServices:_bounds_check(x, y)
   return MathFns.bound(x, 1, self._properties.size.width),
          MathFns.bound(y, 1, self._properties.size.length)
end

function ScenarioServices:_set_random_facing(entity)
   local facing = self.rng:get_int(0, 3)
   entity:add_component('mob'):turn_to(90*facing)
end

return ScenarioServices
