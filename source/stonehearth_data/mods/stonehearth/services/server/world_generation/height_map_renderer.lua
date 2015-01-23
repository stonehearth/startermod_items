local Array2D = require 'services.server.world_generation.array_2D'
local Rect2 = _radiant.csg.Rect2
local Point2 = _radiant.csg.Point2
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region2 = _radiant.csg.Region2
local Region3 = _radiant.csg.Region3
local HeightMapCPP = _radiant.csg.HeightMap

local HeightMapRenderer = class()

function HeightMapRenderer:__init(terrain_info)
   self._terrain_info = terrain_info
   self._tile_size = self._terrain_info.tile_size
   self._terrain = radiant._root_entity:add_component('terrain')

   self._block_types = radiant.terrain.get_block_types()

   -- rock layers
   local mountains_info = self._terrain_info.mountains
   local rock_layers = {}
   local terrain_tags = {
      self._block_types.rock_layer_1,
      self._block_types.rock_layer_2,
      self._block_types.rock_layer_3,
      self._block_types.rock_layer_4,
      self._block_types.rock_layer_5,
      self._block_types.rock_layer_6
   }

   for i, tag in ipairs(terrain_tags) do
      rock_layers[i] = {
         terrain_tag = tag,
         max_height = mountains_info.base_height + mountains_info.step_size*i
      }
   end

   self._rock_layers = rock_layers
   self._num_rock_layers = #rock_layers

   -- add land function table
   self._add_land_functions = {
      plains = self._add_plains_to_region,
      foothills = self._add_foothills_to_region,
      mountains = self._add_mountains_to_region
   }

   self._show_tile_boundaries = radiant.util.get_config('show_tile_boundaries', false)
end

function HeightMapRenderer:add_region_to_terrain(region3, offset_x, offset_y)
   local clipper

   if not self._show_tile_boundaries then
      clipper = Rect2(
         Point2(offset_x, offset_y),
         Point2(offset_x + self._tile_size, offset_y + self._tile_size)
      )
   else
      clipper = Rect2(Point2(0, 0), Point2(0, 0))
   end

   region3:translate(Point3(offset_x, 0, offset_y))
   self._terrain:add_tile_clipped(region3, clipper)
end

function HeightMapRenderer:render_height_map_to_region(region3, height_map, underground_height_map)
   assert(height_map.width == self._tile_size)
   assert(height_map.height == self._tile_size)
   assert(underground_height_map.width == self._tile_size)
   assert(underground_height_map.height == self._tile_size)

   local surface_region = self:_convert_height_map_to_region3(height_map, self._add_land_to_region)
   region3:add_region(surface_region)

   -- will overwrite some cubes from the surface region
   local underground_region = self:_convert_height_map_to_region3(underground_height_map, self._add_mountains_to_region)
   region3:add_region(underground_region)

   local bedrock_region = self:_get_bedrock_region(height_map, 4)
   region3:add_region(bedrock_region)

   region3:optimize_by_merge()
end

function HeightMapRenderer:_convert_height_map_to_region3(height_map, add_fn)
   local region3 = Region3()
   local region2 = Region2()

   self:_convert_height_map_to_region2(region2, height_map)

   for rect in region2:each_cube() do
      local height = rect.tag
      if height > 0 then
         add_fn(self, region3, rect, height)
      end
   end

   return region3
end

function HeightMapRenderer:_convert_height_map_to_region2(region2, height_map)
   assert(height_map.width == height_map.height)
   local height_map_cpp = HeightMapCPP(height_map.width, 1) -- Assumes square map!

   self:_copy_heightmap_to_CPP(height_map_cpp, height_map)
   _radiant.csg.convert_heightmap_to_region2(height_map_cpp, region2)
end

function HeightMapRenderer:_copy_heightmap_to_CPP(height_map_cpp, height_map)
   local row_offset = 0

   for j=1, height_map.height do
      for i=1, height_map.width do
         -- switch from lua height_map coordinates to cpp coordinates
         height_map_cpp:set(i-1, j-1, height_map[row_offset+i])
      end
      row_offset = row_offset + height_map.width
   end
end

function HeightMapRenderer:_get_bedrock_region(height_map, thickness)
   local region3 = Region3()

   region3:add_unique_cube(Cube3(
         Point3(0, -thickness, 0),
         Point3(height_map.width, 0, height_map.height),
         self._block_types.bedrock
      ))

   return region3
end

function HeightMapRenderer:_add_land_to_region(region3, rect, height)
   local terrain_type = self._terrain_info:get_terrain_type(height)

   self._add_land_functions[terrain_type](self, region3, rect, height)
end

function HeightMapRenderer:_add_mountains_to_region(region3, rect, height)
   local rock_layers = self._rock_layers
   local num_rock_layers = self._num_rock_layers
   local i, block_min, block_max
   local stop = false

   block_min = 0

   for i=1, num_rock_layers do
      if (i == num_rock_layers) or (height <= rock_layers[i].max_height) then
         block_max = height
         stop = true
      else
         block_max = rock_layers[i].max_height
      end

      region3:add_unique_cube(Cube3(
            Point3(rect.min.x, block_min, rect.min.y),
            Point3(rect.max.x, block_max, rect.max.y),
            rock_layers[i].terrain_tag
         ))

      if stop then return end
      block_min = block_max
   end
end

function HeightMapRenderer:_add_foothills_to_region(region3, rect, height)
   local foothills_step_size = self._terrain_info.foothills.step_size
   local plains_max_height = self._terrain_info.plains.max_height

   local has_grass = height % foothills_step_size == 0
   local soil_top = has_grass and height-1 or height

   self:_add_soil_strata_to_region(region3, Cube3(
         Point3(rect.min.x, 0,        rect.min.y),
         Point3(rect.max.x, soil_top, rect.max.y)
      ))

   if has_grass then
      region3:add_unique_cube(Cube3(
            Point3(rect.min.x, soil_top, rect.min.y),
            Point3(rect.max.x, height,   rect.max.y),
            self._block_types.grass
         ))
   end
end

function HeightMapRenderer:_add_plains_to_region(region3, rect, height)
   local plains_max_height = self._terrain_info.plains.max_height
   local material = height < plains_max_height and self._block_types.dirt or self._block_types.grass

   self:_add_soil_strata_to_region(region3, Cube3(
         Point3(rect.min.x, 0,        rect.min.y),
         Point3(rect.max.x, height-1, rect.max.y)
      ))

   region3:add_unique_cube(Cube3(
         Point3(rect.min.x, height-1, rect.min.y),
         Point3(rect.max.x, height,   rect.max.y),
         material
      ))
end

local STRATA_HEIGHT = 2
local NUM_STRATA = 2
local STRATA_PERIOD = STRATA_HEIGHT * NUM_STRATA

function HeightMapRenderer:_add_soil_strata_to_region(region3, cube3)
   local y_min = cube3.min.y
   local y_max = cube3.max.y
   local j_min = math.floor(cube3.min.y / STRATA_HEIGHT) * STRATA_HEIGHT
   local j_max = cube3.max.y

   for j = j_min, j_max, STRATA_HEIGHT do
      local lower = math.max(j, y_min)
      local upper = math.min(j+STRATA_HEIGHT, y_max)
      local block_type = j % STRATA_PERIOD == 0 and self._block_types.soil_light or self._block_types.soil_dark

      region3:add_unique_cube(Cube3(
            Point3(cube3.min.x, lower, cube3.min.z),
            Point3(cube3.max.x, upper, cube3.max.z),
            block_type
         ))
   end
end

return HeightMapRenderer
