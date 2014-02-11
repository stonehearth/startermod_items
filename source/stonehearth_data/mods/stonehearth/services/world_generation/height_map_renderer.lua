local Array2D = require 'services.world_generation.array_2D'
local TerrainType = require 'services.world_generation.terrain_type'

local Terrain = _radiant.om.Terrain
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
   self._terrain:set_tile_size(self._tile_size)

   -- rock layers
   local rock_layers = { {}, {}, {} }
   local mountains_info = self._terrain_info[TerrainType.mountains]
   rock_layers[1].terrain_tag = Terrain.ROCK_LAYER_1
   rock_layers[1].max_height  = mountains_info.base_height + mountains_info.step_size
   rock_layers[2].terrain_tag = Terrain.ROCK_LAYER_2
   rock_layers[2].max_height  = rock_layers[1].max_height + mountains_info.step_size
   rock_layers[3].terrain_tag = Terrain.ROCK_LAYER_3
   rock_layers[3].max_height  = rock_layers[2].max_height + mountains_info.step_size
   self.rock_layers = rock_layers
end

function HeightMapRenderer:create_new_region()
   return _radiant.sim.alloc_region() -- returns a C++ Region3Boxed
end

function HeightMapRenderer:add_region_to_terrain(region3_boxed, offset)
   if offset == nil then offset = Point3(0, 0, 0) end
   self._terrain:add_tile(offset, region3_boxed)
end

function HeightMapRenderer:render_height_map_to_region(region3_boxed, height_map)
   assert(height_map.width == self._tile_size)
   assert(height_map.height == self._tile_size)

   local region2 = Region2()
   local height

   self:_convert_height_map_to_region2(region2, height_map)

   region3_boxed:modify(function(region3)
      self:_add_bedrock_to_region(region3, height_map, 4)

      for rect in region2:each_cube() do
         height = rect.tag
         if height > 0 then
            self:_add_land_to_region(region3, rect, height);
         end
      end
   end)
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

function HeightMapRenderer:_add_bedrock_to_region(region3, height_map, thickness)
   region3:add_cube(Cube3(Point3(0, -thickness, 0),
                          Point3(height_map.width, 0, height_map.height),
                          Terrain.ROCK_LAYER_1))
end

function HeightMapRenderer:_add_land_to_region(region3, rect, height)
   local terrain_type = self._terrain_info:get_terrain_type(height)

   -- as we grow TerrainTypes, put this in a table and dynamically call the function

   if terrain_type == TerrainType.plains then
      self:_add_plains_to_region(region3, rect, height)
      return
   end

   if terrain_type == TerrainType.foothills then
      self:_add_foothills_to_region(region3, rect, height)
      return
   end

   if terrain_type == TerrainType.mountains then
      self:_add_mountains_to_region(region3, rect, height)
      return
   end
end

function HeightMapRenderer:_add_mountains_to_region(region3, rect, height)
   local rock_layers = self.rock_layers
   local i, block_min, block_max
   local stop = false

   block_min = 0

   for i=1, #rock_layers do
      if (i == #rock_layers) or (height <= rock_layers[i].max_height) then
         block_max = height
         stop = true
      else
         block_max = rock_layers[i].max_height
      end

      region3:add_cube(Cube3(Point3(rect.min.x, block_min, rect.min.y),
                             Point3(rect.max.x, block_max, rect.max.y),
                             rock_layers[i].terrain_tag))

      if stop then return end
      block_min = block_max
   end
end

function HeightMapRenderer:_add_foothills_to_region(region3, rect, height)
   local foothills_step_size = self._terrain_info[TerrainType.foothills].step_size

   if height % foothills_step_size == 0 then

      region3:add_cube(Cube3(Point3(rect.min.x, 0,        rect.min.y),
                             Point3(rect.max.x, height-1, rect.max.y),
                             Terrain.SOIL))

      region3:add_cube(Cube3(Point3(rect.min.x, height-1, rect.min.y),
                             Point3(rect.max.x, height,   rect.max.y),
                             Terrain.GRASS))
   else
      region3:add_cube(Cube3(Point3(rect.min.x, 0,        rect.min.y),
                             Point3(rect.max.x, height,   rect.max.y),
                             Terrain.SOIL))
   end
end

function HeightMapRenderer:_add_plains_to_region(region3, rect, height)
   local plains_max_height = self._terrain_info[TerrainType.plains].max_height
   local material

   region3:add_cube(Cube3(Point3(rect.min.x, 0,        rect.min.y),
                          Point3(rect.max.x, height-1, rect.max.y),
                          Terrain.SOIL))

   if height < plains_max_height then
      material = Terrain.DIRT
   else 
      material = Terrain.GRASS
   end

   region3:add_cube(Cube3(Point3(rect.min.x, height-1, rect.min.y),
                          Point3(rect.max.x, height,   rect.max.y),
                          material))
end

-----

function HeightMapRenderer:visualize_height_map(height_map)
   local region3_boxed = create_new_region()
   local region2 = Region2()
   local height

   self:_convert_height_map_to_region2(region2, height_map)

   region3_boxed:modify(function(region3)
      for rect in region2:each_cube() do
         height = rect.tag
         if height >= 1 then
            region3:add_cube(Cube3(Point3(rect.min.x, -1,       rect.min.y),
                                   Point3(rect.max.x, height-1, rect.max.y),
                                   Terrain.ROCK_LAYER_1))
         end
         region3:add_cube(Cube3(Point3(rect.min.x, height-1, rect.min.y),
                                Point3(rect.max.x, height,   rect.max.y),
                                Terrain.ROCK_LAYER_1))
      end
   end)
   self.add_region_to_terrain(region3_boxed)
end

function HeightMapRenderer.tesselator_test()
   local terrain = radiant._root_entity:add_component('terrain')
   local region3_boxed = _radiant.sim.alloc_region()
   local height = 10

   region3_boxed:modify(function(region3)
      --region3:add_cube(Cube3(Point3(0, height-1, 0), Point3(16, height, 16), Terrain.GRASS))
      --region3:add_cube(Cube3(Point3(16, height-1, 1), Point3(17, height, 15), Terrain.SOIL))

      region3:add_cube(Cube3(Point3(0, height-1, 0), Point3(256, height, 256), Terrain.GRASS))
      --region3:add_cube(Cube3(Point3(256, height-1, 0), Point3(257, height, 256), Terrain.SOIL))
   end)
   terrain:add_tile(Point3(0, 0, 0), region3_boxed)
end

return HeightMapRenderer
