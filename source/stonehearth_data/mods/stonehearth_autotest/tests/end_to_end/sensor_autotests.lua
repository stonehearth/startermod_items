local Point3 = _radiant.csg.Point3

local sensor_tests = {}

-- the sensor radius is 32.  position the sensor at the center of the
-- 0,0 tile, which means 8 units of slack on either side will be on
-- the fringe.

local TILE_SIZE = 16

local SENSOR_POS = { x = 8, z = 8 }
local SENSOR_RADIUS = 10

local SENSOR_BOX = {
   x0 = SENSOR_POS.x - SENSOR_RADIUS,
   x1 = SENSOR_POS.x + SENSOR_RADIUS,
   z0 = SENSOR_POS.z - SENSOR_RADIUS,
   z1 = SENSOR_POS.z + SENSOR_RADIUS,
}

-- some points around the sensor.  the encoding of the name describes where they are
--
--   INTERIOR means the point lies on one of the tiles which is wholly contained by the sensor.
--   FRINGE means the point lies on one of the tiles which partially overlaps the sensor
--   INSIDE means the point's inside the sensor
--   OUTSIDE means the points' outside the sensor
-- 

local INSIDE_SENSOR_INTERIOR_1 = { x = SENSOR_POS.x + 2,  z = SENSOR_POS.z + 2 }
local INSIDE_SENSOR_INTERIOR_2 = { x = SENSOR_POS.x + 2,  z = SENSOR_POS.z - 2 }
local INSISE_SENSOR_FRINGE_1   = { x = SENSOR_POS.x + SENSOR_RADIUS - 2, z = SENSOR_POS.z + 2 }
local INSIDE_SENSOR_FRINGE_2   = { x = SENSOR_POS.x + SENSOR_RADIUS - 2, z = SENSOR_POS.z - 2 }
local OUTSIDE_SENSOR_FRINGE_1  = { x = SENSOR_POS.x + SENSOR_RADIUS + 2, z = SENSOR_POS.z + 2 }
local OUTSIDE_SENSOR_FRINGE_2  = { x = SENSOR_POS.x + SENSOR_RADIUS + 2, z = SENSOR_POS.z - 2 }
local OUTSIDE_SENSOR_1         = { x = SENSOR_POS.x + SENSOR_RADIUS + 2 + TILE_SIZE, z = SENSOR_POS.z + 2 }
local OUTSIDE_SENSOR_2         = { x = SENSOR_POS.x + SENSOR_RADIUS + 2 + TILE_SIZE, z = SENSOR_POS.z - 2 }

local function check_sensor(autotest, sensor_entity, expected)  
   autotest:sleep(0)
   
   local success = true
   local expected_map = {}
   local sensor = sensor_entity:get_component('sensor_list'):get_sensor('sight')

   for _, entity in pairs(expected) do
      expected_map[entity:get_id()] = entity
   end
   for id, entity in sensor:each_contents() do
      if not expected_map[id] then
         local delta_pos = radiant.entities.get_world_grid_location(entity) -
                           radiant.entities.get_world_grid_location(sensor_entity)
         autotest:log('unexpected entity %s found in sensor (delta pos: %s)', entity, delta_pos)
         success = false
      end
      expected_map[id] = nil
   end
   for id, entity in pairs(expected_map) do
      local delta_pos = radiant.entities.get_world_grid_location(entity) -
                        radiant.entities.get_world_grid_location(sensor_entity)
      autotest:log('could not find expected entity %s in sensor (delta pos: %s)', entity, delta_pos)
      success = false
   end
   if not success then
      autotest:fail('sensor does not contain expected contents')
   end
end

local function move_to(autotest, obj, point)
   radiant.entities.move_to(obj, Point3(point.x, 1, point.z))
   autotest:sleep(0)
end

local function create_default_sensor(autotest)
   return autotest.env:create_entity(SENSOR_POS.x, SENSOR_POS.z, 'stonehearth_autotest:entities:5_unit_sensor')
end

-- make sure a sensor can detect the ground and itself.
function sensor_tests.trivial(autotest)
   local entity = create_default_sensor(autotest)
   check_sensor(autotest, entity, { radiant.get_object(1), entity })
   autotest:success()
end

-- make sure a sensor doesn't detect things out of range.
function sensor_tests.out_of_range(autotest)
   local entity = create_default_sensor(autotest)
   local log = autotest.env:create_entity(OUTSIDE_SENSOR_1.x, OUTSIDE_SENSOR_1.z, 'stonehearth:oak_log')

   local path = {
      OUTSIDE_SENSOR_2,
      OUTSIDE_SENSOR_FRINGE_1,
      OUTSIDE_SENSOR_FRINGE_2,
      OUTSIDE_SENSOR_FRINGE_1,
      OUTSIDE_SENSOR_1,
      OUTSIDE_SENSOR_2,
      OUTSIDE_SENSOR_1,
   }

   local no_log_in_sensor = { radiant.get_object(1), entity }
   for _, point in ipairs(path) do
      move_to(autotest, log, point)
      check_sensor(autotest, entity, no_log_in_sensor)
   end
   autotest:success()
end

function sensor_tests.move_out_of_range(autotest)
   local entity = create_default_sensor(autotest)
   local log = autotest.env:create_entity(OUTSIDE_SENSOR_1.x, OUTSIDE_SENSOR_1.z, 'stonehearth:oak_log')

   local function move_out_of_range(inside, outside)
      autotest:log('moving object from inside to outside sensor range (%d, %d -> %d, %d)', inside.x, inside.z, outside.x, outside.z)
      move_to(autotest, log, inside)
      check_sensor(autotest, entity, { radiant.get_object(1), entity, log })
      move_to(autotest, log, outside)
      check_sensor(autotest, entity, { radiant.get_object(1), entity })
   end
   move_out_of_range(INSISE_SENSOR_FRINGE_1, OUTSIDE_SENSOR_1)
   move_out_of_range(INSISE_SENSOR_FRINGE_1, OUTSIDE_SENSOR_FRINGE_1)
   move_out_of_range(INSIDE_SENSOR_INTERIOR_1, OUTSIDE_SENSOR_1)
   move_out_of_range(INSIDE_SENSOR_INTERIOR_1, OUTSIDE_SENSOR_FRINGE_1)
   autotest:success()
end

function sensor_tests.move_into_range(autotest)
   local entity = create_default_sensor(autotest)
   local log = autotest.env:create_entity(OUTSIDE_SENSOR_1.x, OUTSIDE_SENSOR_1.z, 'stonehearth:oak_log')

   local function move_into_range(outside, inside)
      autotest:log('moving object from outside to inside sensor range (%d, %d -> %d, %d)', outside.x, outside.z, inside.x, inside.z)
      move_to(autotest, log, outside)
      check_sensor(autotest, entity, { radiant.get_object(1), entity })
      move_to(autotest, log, inside)
      check_sensor(autotest, entity, { radiant.get_object(1), entity, log })
   end
   move_into_range(OUTSIDE_SENSOR_1, INSISE_SENSOR_FRINGE_1)
   move_into_range(OUTSIDE_SENSOR_1, INSIDE_SENSOR_INTERIOR_1)
   move_into_range(OUTSIDE_SENSOR_FRINGE_1, INSISE_SENSOR_FRINGE_1)
   move_into_range(OUTSIDE_SENSOR_FRINGE_1, INSIDE_SENSOR_INTERIOR_1)
   autotest:success()
end

function sensor_tests.moving_sensor(autotest)
   local logs = {}
   local spacing = 4
   local radius = SENSOR_RADIUS + (spacing * 2)

   for x = -radius, radius, spacing do
      for z = -radius, radius, spacing do
         local position = {
            x = x + SENSOR_POS.x,
            z = z + SENSOR_POS.z
         }
         table.insert(logs, {
               log = autotest.env:create_entity(position.x, position.z, 'stonehearth:oak_log'),
               position = position
            })
      end
   end

   local function check_moving_sensor(sensor_entity)
      local expected = { radiant.get_object(1), sensor_entity }
      local pos = radiant.entities.get_world_grid_location(sensor_entity)
      for _, entry in ipairs(logs) do
         local log_pos = entry.position
         local dx = log_pos.x - pos.x
         local dz = log_pos.z - pos.z
         if dx >= -SENSOR_RADIUS and dx < SENSOR_RADIUS and
            dz >= -SENSOR_RADIUS and dz < SENSOR_RADIUS then
            table.insert(expected, entry.log)
         end
      end
      check_sensor(autotest, sensor_entity, expected)
   end

   local sensor_entity = create_default_sensor(autotest)
   for dz = -radius, radius do 
      radiant.entities.move_to(sensor_entity, Point3(SENSOR_POS.x, 1, SENSOR_POS.z + dz))
      check_moving_sensor(sensor_entity)
   end
   autotest:success()
end


return sensor_tests
