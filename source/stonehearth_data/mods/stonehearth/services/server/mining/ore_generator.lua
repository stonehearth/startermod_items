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

   local vein_offset_y = rng:get_int(1, 2)
   local start = Point3(0, vein_offset_y, 0)
   local segment, finish

   if has_kink then
      local half_min_length = math.floor(json.vein_length_min*0.5)
      -- -vein_radius because we will extend that far in the pricipal axis when we generate the normal segment
      local kink_index = rng:get_int(half_min_length, length-vein_radius)
      -- min of 12 so that we leave an empty 4x4 column between tunnels. derive this constant later
      local kink_length = rng:get_int(12, half_min_length)

      segment, finish = ore_generator._create_vein_segment(start, kink_index, direction, normal, vein_radius, block_type, false)
      region:add_cube(segment)

      start = finish
      segment, finish = ore_generator._create_vein_segment(start, kink_length, normal, direction, vein_radius, block_type, true)
      region:add_cube(segment)

      start = finish
      -- +1 because our start point along the pricipal axis overlaps the end point from the first segment
      segment, finish = ore_generator._create_vein_segment(start, length-kink_index+1, direction, normal, vein_radius, block_type, true)
      region:add_cube(segment)
   else
      segment, finish = ore_generator._create_vein_segment(start, length, direction, normal, vein_radius, block_type, false)
      region:add_cube(segment)
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

function ore_generator._create_vein_segment(start, length, direction, normal, vein_radius, block_type, connect)
   local finish = start + direction * (length-1)
   if connect then
      -- back up to fill the void in the corner
      start = start - direction * vein_radius
   end

   local corner1 = start - normal * vein_radius
   local corner2 = finish + normal * vein_radius
   local segment = csg_lib.create_cube(corner1, corner2, block_type) 
   return segment, finish
end

return ore_generator
