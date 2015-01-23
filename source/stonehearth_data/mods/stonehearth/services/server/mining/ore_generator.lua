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
   local has_kink = rng:get_real(0, 1)  < 0.50
   local length = rng:get_int(json.vein_length_min, json.vein_length_max)
   local vein_radius = json.vein_radius
   local region = Region3()

   -- chooses a principal axis for the vein to follow
   local direction, normal = ore_generator._get_random_direction(rng)

   local segment_start = Point3(0, 2, 0)
   local segment, segment_end

   if has_kink then
      local half_min_length = math.floor(json.vein_length_min*0.5)
      local kink_index = rng:get_int(half_min_length, length)
      local kink_length = rng:get_int(12, half_min_length)

      segment, segment_end = ore_generator._create_vein_segment(segment_start, kink_index, direction, normal, vein_radius, block_type, rng)
      region:add_region(segment)

      segment_start = segment_end
      segment, segment_end = ore_generator._create_vein_segment(segment_start, kink_length, normal, direction, vein_radius, block_type, rng)
      region:add_region(segment)

      segment_start = segment_end
      segment, segment_end = ore_generator._create_vein_segment(segment_start, length - kink_index, direction, normal, vein_radius, block_type, rng)
      region:add_region(segment)
   else
      segment, segment_end = ore_generator._create_vein_segment(segment_start, length, direction, normal, vein_radius, block_type, rng)
      region:add_region(segment)
   end

   return region
end

function ore_generator._get_random_direction(rng)
   local angle = rng:get_int(0, 3) * 90
   local direction = radiant.math.rotate_about_y_axis(Point3.unit_x, angle):to_closest_int()
   local normal = radiant.math.rotate_about_y_axis(Point3.unit_z, angle):to_closest_int()
   local sign = rng:get_int(0, 1) * 2 - 1
   normal = normal * sign
   return direction, normal
end

function ore_generator._create_vein_segment(start_location, length, direction, normal, vein_radius, block_type, rng)
   local horizontal_drift_chance = 0.20
   local vertical_drift_chance = 0.20
   local min_drift_interval = 2
   local last_drift = -min_drift_interval
   local region = Region3()
   local location = start_location

   local slice = ore_generator._create_vein_slice(location, normal, vein_radius, block_type)
   region:add_cube(slice)

   for i=2, length do
      location = location + direction

      if i - last_drift >= min_drift_interval and
         i <= length - min_drift_interval then
         -- horizontal perturbation
         -- if rng:get_real(0, 1) < horizontal_drift_chance then
         --    local sn = rng:get_int(0, 1) * 2 - 1
         --    location = location + normal*sn
         --    last_drift = i
         -- end

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

      slice = ore_generator._create_vein_slice(location, normal, vein_radius, block_type)
      region:add_cube(slice)
   end

   return region, location
end

function ore_generator._create_vein_slice(location, normal, radius, block_type)
   local p0 = location - normal * radius
   local p1 = location + normal * radius
   return csg_lib.create_cube(p0, p1, block_type)
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

return ore_generator
