local constants = require 'constants'
local csg_lib = require 'lib.csg.csg_lib'
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

local ore_generator = {}

function ore_generator.create_ore_network(kind, properties, rng)
   local rng = rng or _radiant.csg.get_default_rng()
   local block_type = radiant.terrain.get_block_types()[kind]
   local json = properties
   local region = Region3()

   local mother_lode = ore_generator._create_mother_lode(block_type, json, rng)
   region:add_region(mother_lode)

   local num_veins = rng:get_int(json.num_veins_min, json.num_veins_max)
   for i = 1, num_veins do
      local vein = ore_generator._create_mineral_vein(block_type, json, rng)
      region:add_region(vein)
   end

   return region
end

function ore_generator._create_mother_lode(block_type, json, rng)
   local usable_slice_height = constants.mining.Y_CELL_SIZE - 1
   local core_radius_x = rng:get_int(json.mother_lode_radius_min, json.mother_lode_radius_max)
   local core_radius_z = rng:get_int(json.mother_lode_radius_min, json.mother_lode_radius_max)
   local region = Region3()

   region:add_cube(
         Cube3(
            Point3(-core_radius_x, 0, -core_radius_z),
            Point3(core_radius_x+1, usable_slice_height, core_radius_z+1),
            block_type
      ))

   local lobe_radius_x = core_radius_x + 1
   local lobe_radius_z = core_radius_z + 1
   region:add_cube(
         Cube3(
            Point3(-lobe_radius_x, 1, -lobe_radius_z),
            Point3(lobe_radius_x+1, usable_slice_height-1, lobe_radius_z+1),
            block_type
      ))

   return region
end

function ore_generator._create_mineral_vein(block_type, json, rng)
   local length = rng:get_int(json.vein_length_min, json.vein_length_max)
   local vein_radius = json.vein_radius
   local horizontal_drift_chance = 0.20
   local vertical_drift_chance = 0.20
   local min_drift_interval = 2
   local last_drift = -min_drift_interval

   -- chooses a principal axis for the vein to follow
   local direction, normal = ore_generator._get_random_direction(rng)
   -- chooses the angle the vein travels relative to the axis
   local bias_vector, bias_count = ore_generator._get_direction_bias(normal, rng)
   local location = Point3(0, 2, 0)
   local region = Region3()

   for i=1, length do
      local slice = ore_generator._create_vein_slice(location, normal, vein_radius, block_type)
      region:add_cube(slice)

      -- if rng:get_real(0, 1) < 0.50 then
      --    local cube = ore_generator._create_ore_point(location, block_type, rng)
      --    region:add_cube(cube)
      -- end

      location = location + direction

      -- every bias_count, add the bias_vector
      if i % bias_count == 0 then
         location = location + bias_vector
      end

      if i - last_drift >= min_drift_interval then
         -- horizontal perturbation
         if rng:get_real(0, 1) < horizontal_drift_chance then
            local sn = rng:get_int(0, 1) * 2 - 1
            location = location + normal*sn
            last_drift = i
         end

         -- vertical perturbation
         if rng:get_real(0, 1) < vertical_drift_chance then
            local dy = rng:get_int(0, 1) * 2 - 1
            local elevation = location.y
            local new_elevation = elevation + dy
            -- keep the vein in the same slice since it's hard to mine up
            if ore_generator._stays_within_slice(elevation, new_elevation) then
               location.y = new_elevation
               last_drift = i
            end
         end
      end
   end

   return region
end

function ore_generator._create_ore_point(location, block_type, rng)
   local sx = rng:get_int(0, 1) * 2 - 1
   local sz = rng:get_int(0, 1) * 2 - 1
   local dx = rng:get_int(8, 28) * sx
   local dz = rng:get_int(8, 28) * sz
   local point = location + Point3(dx, 0, dz)
   return Cube3(point, point + Point3.one, block_type)
end

function ore_generator._stays_within_slice(elevation, new_elevation)
   local y_cell_size = constants.mining.Y_CELL_SIZE
   local current_slice = math.floor(elevation / y_cell_size)
   local new_slice = math.floor(new_elevation / y_cell_size)

   if new_slice ~= current_slice then
      return false
   end

   -- exclude the ceiling from the slice since it's hard to mine ceilings
   if new_elevation % y_cell_size == y_cell_size - 1 then
      return false
   end

   return true
end

function ore_generator._get_random_direction(rng)
   local angle = rng:get_int(0, 3) * 90
   local direction = radiant.math.rotate_about_y_axis(Point3.unit_x, angle):to_closest_int()
   local normal = radiant.math.rotate_about_y_axis(Point3.unit_z, angle):to_closest_int()
   return direction, normal
end

-- returns a bias vector and how often to add it
function ore_generator._get_direction_bias(normal, rng)
   -- angle will clip at 26.57 degrees, because count has a minimum of 2 to avoid 45 degree angles
   local angle = rng:get_int(-22, 22)

   -- get the cotangent (x/y) of the angle. basically how often to move forward before moving sideways.
   local count = radiant.math.round(1 / math.tan(math.abs(math.rad(angle))))
   -- minimum of 2 to exclude 45 degree angles
   -- maximum so that we don't leak infinity when angle is close to 0
   count = radiant.math.bound(count, 2, 1e+9)

   local sign = angle >= 0 and 1 or -1
   local vector = normal * sign

   return vector, count
end

function ore_generator._create_vein_slice(location, normal, radius, block_type)
   local p0 = location - normal * radius
   local p1 = location + normal * radius
   return csg_lib.create_cube(p0, p1, block_type)
end

return ore_generator
